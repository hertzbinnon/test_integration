import signal, os
import sys
import xml.etree.ElementTree as ET
import shlex
import time,_thread
from multiprocessing import cpu_count
from subprocess import *
import logging  
import logging.handlers
import streamer
import wsgi 
import setproctitle
import binding

#import pdb
#pdb.set_trace()
#command line and envp process
main_name = 'smsd'
setproctitle.setproctitle(main_name)
argv = sys.argv
if len(argv) < 4:
	exit()
os.environ['VLC_PLUGIN_PATH']='/home/smsd/modules'
#os.environ['LD_LIBRARY_PATH']='/home/smsd/modules'

#log process
DEBUG='0'
stdout_debug=sys.argv[1]+'/log/smsd-out-err.log'
stdout_null='/dev/null'
if sys.argv[3] == '1': 
	STD_OUT = stdout_debug
	DEBUG='1'
else:
	STD_OUT = stdout_null
	DEBUG='0'
try:
	with open(STD_OUT, 'w') as logfile:
		os.dup2(logfile.fileno(), sys.stdout.fileno())
		os.dup2(logfile.fileno(), sys.stderr.fileno())
except Exception as e:
	print('Failed to open file:',e)
	exit()
vlc_input__timeout = 3
LOG_FILE = sys.argv[1]+'/log/smsd.log'
handler = logging.handlers.RotatingFileHandler(LOG_FILE, maxBytes = 1024*1024, backupCount = 1000)    
#fmt = '%(asctime)s - %(filename)s:%(lineno)s - %(name)s - %(message)s'
fmt = '%(asctime)s - %(name)s - %(message)s'
formatter = logging.Formatter(fmt) 
handler.setFormatter(formatter)
logger = logging.getLogger('smsd')
logger.addHandler(handler) 
logger.setLevel(logging.DEBUG)
logger.debug("smsd start")

event_queue=None
socket=None
engine_process= 0 # 1 -> child process
main_exit = 0
task_list={} # {taskid:[srcid,ppid->int,pidfile,trans-id,exitcode,restart-count,time-to-streaming-for-a-srcid]}
transcation_list = {} #
subprocess_format = 'itv-channel-%s'
# cpu,memmery protect
def system_rlimit(pid):
	error = 0
	pagesize=os.sysconf('SC_PAGE_SIZE')/1024 # 4K  
	try:
		rss=int(open('/proc/'+pid+'/statm','r').readline().split()[1])*pagesize/1024
		if rss > 4 * 1024:
			logger.debug("Channel  %s is exiting" % task[0])
			error = 1
		""" 
		fd=open('/proc/'+pid+'/stat','rb')
		line = fd.readline().split()
		runtime = (int(line[13]) + int(line[14])) * 10 / 1000 
		if len(os.listdir('/proc/'+pid+'/task')) < 8 and runtime >= 0:
			logger.debug("run time = %d",runtime)
			error = 2
		"""
	except IOError as e:
		logger.debug("open file failed ", e)
	return error

def check_waitpid(pid):
	ret = 0
	status = 0
	try:
		pid,status = os.waitpid(pid,os.WNOHANG)
		#print "pid ",pid, "running",status
	except Exception as e:
		#status = status / 256
		print(pid, " has exit status" ,status)
		ret = 1
	return ret
		
# signal process
def handler(signum, frame):
	#logger.debug('signal %d (engine_process = %d)', signum,engine_process);
	if signum == signal.SIGCHLD:
		#logger.debug("process signal %d " ,signal.SIGCHLD)
		while 1:
			try:
				pid,status  = os.waitpid(-1,os.WNOHANG)
				#logger.debug("pid %d exit code %d" ,pid,status)
				if pid <= 0:
					#logger.debug("Child process exist, but no exit")
					break
				status_div = status / 256
				status_rem = status % 256
				status = max(status_div , status_rem)
				#print "pid", pid, "exit"," status", status, "engine_process", engine_process
				logger.debug("pid %d,exitcode %d,engine_process %d", pid,  status,  engine_process)
				for id in task_list:
					if task_list[id][1] == pid:
						task_list[id][1] = -1
						task_list[id][4] = status
					#logger.debug("signal %d channel %s ",signum, id)
						break
			except Exception as e:
				print('Child Process',e)
				#print "No child !!!!!!!!!!!"
				break
	elif signum == signal.SIGINT or signum == signal.SIGTERM or signum == signal.SIGABRT:
		logger.debug('signal %d (engine_process = %d)', signum,engine_process);
		if engine_process: 
			exit(0)
		#print 'main process will exit'
		global main_exit
		main_exit = 1
			
signal.signal(signal.SIGINT,	handler)
signal.signal(signal.SIGTERM,	handler)
signal.signal(signal.SIGABRT,	handler)
### Note !!!!!!
#signal.signal(signal.SIGCHLD,signal.SIG_IGN) 
signal.signal(signal.SIGCHLD,	handler) 

def event_Func(event,trans):
	errno = -1 
	if event.type == binding.EventType.MediaPlayerEncounteredError_I:
		errno = 2001
	elif event.type == binding.EventType.MediaPlayerEncounteredError_N:
		errno = 2002
	elif event.type == binding.EventType.MediaPlayerEncounteredError_T:
		errno = 2003
	print("errno= ",errno)
	_thread.start_new_thread(wsgi.server_async_notify,(transcation_list[trans],None,errno))

def fork_engine(ids, streamer,trans=None):
	task = {}
	for id in ids:
		pid = os.fork()
		if pid > 0:
			pidfile = open('/tmp/'+id+'.smsd', 'w')
			pidfile.write(str(pid))
			pidfile.closed
			task[id] = ['srcid',pid,pidfile.name,trans,0,0,time.time()]
			del pidfile
			continue	
		elif pid == 0:
			#TODO delete wsgi
			global socket
			if socket is not None:
				socket.close()
				del socket
			global engine_process
			engine_process = 1
			timing = streamer.get_timing(id)

			subLOG_FILE = sys.argv[1]+'/log/itv-channel-'+id+'/smsd-error.log'
			try:
				with open(subLOG_FILE , 'w') as sublogfile:
					#sys.stdout.closed
					#sys.stderr.closed
					#sys.stdout = sublogfile
					#sys.stderr = sublogfile
					os.dup2(sublogfile.fileno(), sys.stdout.fileno())
					os.dup2(sublogfile.fileno(), sys.stderr.fileno())
			except Exception as e:
				print('subprocess failed to open ',e)
			subhandler = logging.handlers.RotatingFileHandler(subLOG_FILE, maxBytes = 10*1024*1024, backupCount = 1000)    
			subfmt = '%(asctime)s - %(name)s - %(message)s'
			subformatter = logging.Formatter(subfmt) 
			subhandler.setFormatter(subformatter)
			sublogger = logging.getLogger('channel')
			sublogger.addHandler(subhandler) 
			sublogger.setLevel(logging.DEBUG)

			streamer.start(id)
			streamer.set_event(id,2001,event_Func,trans)
			streamer.set_event(id,2002,event_Func,trans)
			streamer.set_event(id,2003,event_Func,trans)
			sublogger.debug("channel-start trans_list(len)=%d",len(transcation_list))
			pid = str(os.getpid())
			setproctitle.setproctitle(subprocess_format % id)
			time.sleep( (int(streamer.get_caching(id)) / 1000.0) )
			restart = 0

			while 1:
				time.sleep(0.01)
				#select.select('','','',timeout)
				restart = 0
				status = streamer.check_status(id).value
				#sublogger.debug("streaming status %d",status)
				#if status == 6 or status == 7:
				"""if status != 3:
					sublogger.debug("[<%s> id : %s]streaming status is %d",pid,id,status)
					#print "**** exit", pid
					exit(3)
					streamer.restart(id)
					time.sleep(3)
					restart = 1"""
				curtime = time.strftime('%H:%M:%S',time.localtime())
				#print "waiting", curtime,' ', timing
				if curtime == timing:
					sublogger.debug("[<%s> id : %s]streaming timing restart",pid,id)
					exit(101)
					"""streamer.restart(id)
					time.sleep(3)
					restart = 1"""
				ret = system_rlimit(pid)
				if restart == 0 and  ret != 0 and status == 3:
					sublogger.debug("[<%s> id :%s]streaming limit error %d",pid,id,ret)
					exit()
					"""streamer.restart(id)
					time.sleep(3)"""
		else:
			task['empty'] = ['srcid',-1,'pidfile',trans,0,0,time.time()]
			#logger.debug("Child process fork failed")	
			print("Child process fork failed")
	return task
		
def event_handler(args_list):
		#"""args_list= {'title':'CCTV1','master':{'taskid':'id','taskid':'id'},'slave':{}}"""
		#print 'event handler:  \n',args_list
		#try:
			global transcation_list 
			global task_list
			global DEBUG
			error = 0
			if 'transid' in args_list and args_list['transid'] in transcation_list or \
				'requestid' in args_list and args_list['requestid'] in transcation_list:
				print("transcation(requestid) id is exist")
				error = 102
			elif args_list['action'] == 'encode':	
				#print("xxx",next(iter(args_list['master'])))#python2
				id = list(args_list['master'])[0]
				print(id)
				if id in task_list:
					print("task id is exist")
					error = 102
				else:
					transcation_list[args_list['transid']] = args_list
					transcation_list[args_list['transid']]['switch'] = 'master' 
					transcation_list[args_list['transid']]['switch_next'] = 'slave' 
					transcation_list[args_list['transid']]['can_switch'] = 'no' 
					transcation_list[args_list['transid']]['exception'] = 0
					transcation_list[args_list['transid']]['server_sync'] = 'no'
					transcation_list[args_list['transid']]['timeout'] = int(args_list['timeout'])
					if int(args_list['timeout']) > 2 * len(args_list['slave']) * vlc_input__timeout:
						transcation_list[args_list['transid']]['timeout_left'] = int(args_list['timeout']) 
					else:
						transcation_list[args_list['transid']]['timeout_left'] =  2 * len(args_list['slave']) * vlc_input__timeout
					if 'slave' in transcation_list[args_list['transid']] and len(transcation_list[args_list['transid']]['slave']) != 0:
						transcation_list[args_list['transid']]['can_switch'] = 'yes' 
						transcation_list[args_list['transid']]['slave_index'] = [0,len(args_list['slave'])]# (slave_index,slave_length)
					else:
						transcation_list[args_list['transid']]['slave_index'] = [0,0]# (slave_index,slave_length)
					transcation_list[args_list['transid']]['source'] = transcation_list[args_list['transid']]['master']
					#transcation_list[args_list['transid']]['srcid']=transcation_list[args_list['transid']]['master'][id][0]['srcid']
					
			elif args_list['action'] == 'reset':	
				error = 404
				for trans in transcation_list: 
					tr_id = list(transcation_list[trans]['master'])[0]
					tk_id =list(args_list['master'])[0]
					if  tr_id == tk_id:
						transcation_list[trans]['state'] = 'cancel'
						transcation_list[args_list['transid']] = args_list
						transcation_list[args_list['transid']]['switch'] = 'master' 
						transcation_list[args_list['transid']]['switch_next'] = 'slave' 
						transcation_list[args_list['transid']]['can_switch'] = 'no' 
						transcation_list[args_list['transid']]['exception'] = 0
						transcation_list[args_list['transid']]['server_sync'] = 'no'
						transcation_list[args_list['transid']]['timeout'] = int(args_list['timeout'])
						if int(args_list['timeout']) > 2 * len(args_list['slave']) * vlc_input__timeout:
							transcation_list[args_list['transid']]['timeout_left'] = int(args_list['timeout']) 
						else:
							transcation_list[args_list['transid']]['timeout_left'] =  2 * len(args_list['slave']) * vlc_input__timeout
						if 'slave' in transcation_list[args_list['transid']] and len(transcation_list[args_list['transid']]['slave']) != 0:
							transcation_list[args_list['transid']]['can_switch'] = 'yes'
							transcation_list[args_list['transid']]['slave_index'] = [0,len(args_list['slave'])]# (slave_index,slave_length)
						else:
							transcation_list[args_list['transid']]['slave_index'] = [0,0]# (slave_index,slave_length)
						transcation_list[args_list['transid']]['source'] = transcation_list[args_list['transid']]['master']
						error = 0
						#print 'reset after',args_list
						break
				pass
			elif args_list['action'] == 'cancel': #####	
				error = 0
				if args_list['type'] != 'source':
					for trans in transcation_list: 
						id = list(transcation_list[trans]['master'])[0]
						if id == args_list['guid']:
							transcation_list[trans]['state'] = 'cancel'
							error = 0
							break
				else:
					for trans in transcation_list:
						id = args_list['sourceid']
						task_id = list(transcation_list[trans]['master'])[0]
						sourceid=transcation_list[trans]['master'][task_id][0]['srcid']
						if sourceid == id:
							transcation_list[trans]['state'] = 'cancel'
							error = 0
					pass
			elif args_list['action'] == 'status':######	
				error = 404
				for trans in transcation_list: 
					id = list(transcation_list[trans]['master'])[0]
					if id == args_list['guid']:
						args_list['state'] = transcation_list[trans]['state']
						args_list['resultmsg'] = transcation_list[trans]['running']
						args_list['srcid'] = transcation_list[trans]['source'].values()[0][0]['srcid'] 
						error = 0
						break
				pass
			elif args_list['action'] == 'request':# setup	
				error = 404
				for trans in transcation_list: 
					id = list(transcation_list[trans]['master'])[0]
					if id == args_list['guid']:
						args_list['org_request'] = transcation_list[trans]['org_request']
						error = 0
				pass
			elif args_list['action'] == 'analyze':	
				pass
			elif args_list['action'] == 'list':	
				tasks=[]
				for trans in transcation_list:
					if 'sourceid' in args_list and args_list['sourceid'] != None:
						id = args_list['sourceid']
						in_id = list(transcation_list[trans]['master'])[0]
						sourceid=transcation_list[trans]['master'][in_id][0]['srcid']
						args_list['title'] = transcation_list[trans]['title']
						if sourceid == id:
							tasks.append((list(transcation_list[trans]['master'])[0], transcation_list[trans]['state']))
					else:
							tasks.append((list(transcation_list[trans]['master'])[0],transcation_list[trans]['state']))
					if len(tasks) > 0 :
						args_list['return'] = tasks
					else:
						error = 2
			elif args_list['action'] == 'heartbeat':	
				args_list['total'] = str(len(transcation_list))
				args_list['executing'] = str(len(task_list))
				pass
			elif args_list['action'] == 'start':	
				error = 2
				for trans in transcation_list: 
					id = list(transcation_list[trans]['master'])[0]
					if id == args_list['taskid']:
						transcation_list[trans]['state'] = 'start'
						error = 0
						break
			elif args_list['action'] == 'stop':	
				error = 2
				for trans in transcation_list: 
					id = list(transcation_list[trans]['master'])[0]
					if id == args_list['taskid']:
						transcation_list[trans]['state'] = 'stop'
						error = 0
						break
			elif args_list['action'] == 'config':
				print('action config')
				error = 2
				if args_list['debug'] != DEBUG:
					DEBUG_OUT = stdout_null
					if args_list['debug'] != '0':
						DEBUG_OUT = stdout_debug
					try:
						with open(DEBUG_OUT, 'a') as logfile:
							os.dup2(logfile.fileno(), sys.stdout.fileno())
							os.dup2(logfile.fileno(), sys.stderr.fileno())
						DEBUG=args_list['debug'] 
						error = 0
					except Exception as e:
						print('failed to open logfile',e)
			else:
				pass
		#except Exception as e:
		#	print("Internal error:",e)
		#	error = -1

		#return error

#trans_timeout=10 
def main_loop(task_list,event_queue):
	while 1:
		#print os.getpid(), '---------------------------------------------'
		#time.sleep(1.0)
		global main_exit
		if main_exit == 1:
			for id in task_list:
				#print 'kill', task_list[id][1]
				os.kill(task_list[id][1],9)
				#os.kill(task_list[id][1],signal.SIGTERM)
				time.sleep(0.5)
				#os.remove(task_list[id][2])
			exit()
			
		for id in task_list:
			pid = task_list[id][1]
			if pid == -1 or check_waitpid(pid) :
				#print 'task' ,id, 'exit' 
				logger.debug("******* Task is exit id = %s,exitcode = %d" , id, task_list[id][4])
				print("******* Task is exit id = " , id, "exitcode",task_list[id][4])
				trans = task_list[id][3]
				_thread.start_new_thread(wsgi.server_async_notify,(transcation_list[trans],task_list[id][4],2000))
				transcation_list[trans]['exception'] += 1#about 100 * 0.01
				if  task_list[id][4] != 197 and time.time() - task_list[id][6] < 15:
					task_list[id][5] += 1 
				if transcation_list[trans]['switch'] !=  transcation_list[trans]['switch_next'] and transcation_list[trans]['can_switch'] == 'yes' and task_list[id][4] == 197 or task_list[id][5]==3:
					print("switching from ", transcation_list[trans]['switch']," to ",transcation_list[trans]['switch_next'])
					logger.debug("switching from %s to %s ", transcation_list[trans]['switch'],transcation_list[trans]['switch_next'])
					# slave is a list of sources, to switch in turn
					slave = {}	
					if transcation_list[trans]['switch_next'] == 'slave':
						slave_index = transcation_list[trans]['slave_index'] 
						slave = transcation_list[trans]['slave'][ slave_index[0] ]
						slave_index[0] = (slave_index[0] + 1) % slave_index[1]
					else:
						slave = transcation_list[trans]['master']
					#print "Test: ",slave
					id = list(slave)[0]
					arch = streamer.get_encoders_arch(slave[id])
					streamer.add_engines(slave,arch)
					transcation_list[trans]['source'] = slave  
					t = transcation_list[trans]['switch_next']
					transcation_list[trans]['switch_next'] = transcation_list[trans]['switch']
					transcation_list[trans]['switch'] = t
					_thread.start_new_thread(wsgi.server_async_notify,(transcation_list[trans],slave,2004))
					task_list[id][5]=0
				#print task_list
				#logger.debug(task_list[id])
				id = list(transcation_list[trans]['source'])[0]
				task_ = fork_engine([id],streamer,trans)
				task_list[id][0] = transcation_list[trans]['source'][id][0]['srcid']
				task_list[id][1] = task_[id][1]
				task_list[id][2] = task_[id][2]
				task_list[id][3] = task_[id][3]
				task_list[id][4] = task_[id][4]
				#task_list[id][5] = task_[id][5]
				task_list[id][6] = task_[id][6]
				if transcation_list[trans]['exception'] >  2 * transcation_list[trans]['slave_index'][1]:#about 100 * 0.01
					if transcation_list[trans]['server_sync'] == 'yes':
						transcation_list[trans]['exception'] = 0 #about 100 * 0.01
					default=None
					_thread.start_new_thread(wsgi.server_async_thread,(transcation_list[trans],default))
				time.sleep(wsgi.EventTick) # prevent fast setup 

		for trans in transcation_list: 
			if transcation_list[trans]['state'] == 'stop':
				if transcation_list[trans]['running'] == 'yes':
					transcation_list[trans]['running'] = 'no'
					id = list(transcation_list[trans]['master'])[0]
					pid = task_list[id][1]
					del task_list[id]	
					#del transcation_list[trans]
					os.kill(pid, 9)

		for trans in transcation_list: 
			if transcation_list[trans]['state'] == 'cancel':
				if transcation_list[trans]['running'] == 'yes':
					print('cancel',trans)
					id = list(transcation_list[trans]['master'])[0]
					pid = task_list[id][1]
					del task_list[id]	
					os.kill(pid, 9)
					print("Delete task", id)
					logger.debug("Delete task %s",id)	
				del transcation_list[trans]
				print("Delete transcation", trans)
				logger.debug("Delete transcation %s",trans)	
				break

		for trans in transcation_list: 
			if transcation_list[trans]['state'] == 'start':
				if transcation_list[trans]['running'] == 'no':
					print('start',trans)
					id = list(transcation_list[trans]['master'])[0]
					arch = streamer.get_encoders_arch(transcation_list[trans]['master'][id])
					streamer.add_engines(transcation_list[trans]['master'],arch)
					task_list[id]=fork_engine([id],streamer,trans)[id]
					task_list[id][0] = transcation_list[trans]['master'][id][0]['srcid']
					transcation_list[trans]['running'] = 'yes'

		for trans in transcation_list: 
			if transcation_list[trans]['server_sync'] == 'no':
				transcation_list[trans]['timeout_left'] -= wsgi.EventTick
				#logger.debug('timeout left: %d',transcation_list[trans]['timeout_left'])
				if transcation_list[trans]['timeout_left'] < 0 :
					#logger.debug('stream exception: %d',transcation_list[trans]['exception'])
					transcation_list[trans]['server_sync'] = 'wait'
					default=None
					_thread.start_new_thread(wsgi.server_sync_thread,(transcation_list[trans],default))
					
		event = wsgi.wsgi_get_event(event_queue)	
		if event:
			wsgi.transcation_process(event,event_handler)
		else:
			time.sleep(wsgi.EventTick) # prevent cpu 



if __name__ == '__main__':
	pidfile = open('/tmp/smsd.pid','w')
	pidfile.write(str(os.getpid()))
	pidfile.closed
	del pidfile
	for pf in os.listdir('/tmp'):
		if pf[-5:] == '.smsd':
			print("Streaming server is exited unnormally ")
			with open('/tmp/'+pf) as f:
				try:
					pid = f.read()
					os.kill(int(pid),signal.SIGTERM)
					os.remove('/tmp/'+pf)
				except Exception as e:
					print('process not exist',e)
					pass
	config_file = '/etc/itvencoder/itvencoder.xml'
	try:
		with open(config_file,"r") as f:
			pass
	except Exception as e:
		print('Local config is not exist',e)
		config_file=None
	streamer = streamer.StreamingMedia(None)
	args_lists=wsgi.parser_config(config_file)
	for args_list in args_lists:
		event_handler(args_list)
	#print('start')
	#exit()
	#ids = ['4','3']
	#task_list = fork_engine(ids,streamer)
	http_port = int(sys.argv[2])
	event_queue,socket = wsgi.wsgi_init(http_port)
	main_loop(task_list,event_queue)

