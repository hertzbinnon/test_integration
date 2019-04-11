import pdb
import sys
import time,_thread
sys.path.append('/home/hebin/.temp/sms/vlc-2.0.0/release-python3/deps/sources')
import eventlet
from eventlet import wsgi
import xml.etree.ElementTree as ET
from queue import Queue

#import threading_queue
import io
import threading
#import httplib 
import http.client

cpu = '0000'
total = '0000'
executing = '0000'
inband = '0000'
outband = '0000'
memory = '0000'
version = '1.1.1'

"""Start  """

xml_header='<?xml version="1.0" encoding="UTF-8"?>'
response_body_common='\r\n<response>\r\n<status>%d</status>\r\n<message>ok</message>\r\n</response>\r\n'

def transcation_heartbeat(args_list):
	error = 0
	args_list['version'] = version 
	args_list['total'] = total
	args_list['executing'] = executing
	args_list['inband'] = inband 
	args_list['outband'] = outband
	args_list['memory'] = memory
	args_list['cpu'] = cpu

	return error

def transcation_encode(args_list):
	args_list['version'] = version 

def transcation_reset(args_list):
	args_list['version'] = version 

def transcation_cancel(args_list):
	args_list['version'] = version 

def transcation_status(args_list):
	args_list['resultmsg'] = '' 
	args_list['resultcode'] = '0' # 1 if running 

def transcation_request(args_list):
	args_list['version'] = version 

def transcation_list(args_list):
	pass
	#if 'sourceid' in args_list:
		#args_list['sourceid'] = "idall"
		

def transcation_analyze(args_list):
	args_list['version'] = version 
def transcation_config(args_list):
	args_list['version'] = version 

def transcation_process(cmd, event_handler=None):
	error = 0;
	if cmd[0]['action'] == 'heartbeat':
		error = transcation_heartbeat(cmd[0]);
	elif cmd[0]['action'] == 'encode':
		transcation_encode(cmd[0]);
	elif cmd[0]['action'] == 'reset':
		transcation_reset(cmd[0]);
	elif cmd[0]['action'] == 'cancel':
		transcation_cancel(cmd[0]);
	elif cmd[0]['action'] == 'start':
		transcation_cancel(cmd[0]);
	elif cmd[0]['action'] == 'stop':
		transcation_cancel(cmd[0]);
	elif cmd[0]['action'] == 'status':
		transcation_status(cmd[0]);
	elif cmd[0]['action'] == 'request':
		transcation_request(cmd[0]);
	elif cmd[0]['action'] == 'list':
		transcation_list(cmd[0]);
	elif cmd[0]['action'] == 'analyze':
		transcation_analyze(cmd[0]);
	elif cmd[0]['action'] == 'config':
		transcation_config(cmd[0]);
	else:
		print(" error ")
	
	if event_handler :
		error = event_handler(cmd[0])

	cmd[0]['request'] = error
	#print 'TRANSCATION Complete\n'
	#print 'enqueue x'
	cmd[1].release()
	#print 'dequeue y'

def parser_common(args,xml):
	context = ET.iterparse(io.StringIO(xml),events=('start','end'))
	for event,elem in context:
		if event == 'start':
			args.update({elem.tag:elem.text})
	#print args.items()
	return args

def parser_heartbeat(xml):
	args_list = {
		'mode':'none',
		'action':'heartbeat',
		'version':'none',
		'total':'none',
		'executing':'none',
		'inband':'none',
		'outband': 'none',
		'memory': 'none',
		'cpu': 'none',
		}
	return (0 ,parser_common(args_list,xml))

def et_find(root,name):
	try:	
		value = root.find(name).text
	except:
		print('Warnning:',name, 'not be found')
		return ''
	return value

def _parser_config(xml):
		args_list = {'action':'config','debug':'0'}
		error = 0
		root = ET.fromstring(xml)
		options = root.find('options')
		for var in options.getiterator('var'):
			args_list[var.attrib.get('name').strip()] = var.text.strip()
		return (error,args_list)

def parser_config(config_xml):
	args_lists = []
	tree = ET.parse(config_xml)
	root = tree.getroot()
	channel_list = root.findall('channels')[0];
	channels = [ele for ele in channel_list.findall('channel')]
	for chan in channels:
		error,args_list = parser_encode(chan,'encode')
		if error == 0:
			args_list['replyaddr'] = root.find('replyaddr').text
			args_list['notifyaddr'] = root.find('notifyaddr').text
			args_list['nodeid'] = root.find('nodeid').text
			args_list['mode'] = root.find('mode').text
			args_list['type'] = root.find('type').text
			args_list['transid'] = chan.find('guid').text
			args_lists.append(args_list)
	return args_lists

def parser_encode(xml,action):
		args_list = {'mode':'none','action':action,'type':'none',
			'streams':[],#usage_param:[   {id:({source},[(video,audio,output),(...),...])},    ]
			'master':{},#usage_param:{id:({},[(),(),...])}
			'slave':[],#usage_param:[{id:({},[(),(),...])},{id:({},[(),(),...])}]
			'org_request':xml,}
		error = 0
	#try:
		#tree = ET.parse(xml)
		#root = tree.getroot()
		if isinstance(xml,str):
			root = ET.fromstring(xml)
			args_list['replyaddr'] = root.find('replyaddr').text
			args_list['notifyaddr'] = root.find('notifyaddr').text
			args_list['nodeid'] = root.find('nodeid').text
			args_list['mode'] = root.find('mode').text
			args_list['type'] = root.find('type').text
			args_list['transid'] = root.find('requestid').text
		else:
			root = xml
			args_list['org_request']= 'Warnning!!!,this is local config'
		taskid=root.find('guid').text
		print("-----------source description---------------")
		sources_token = root.findall('sources')[0]
		source_token = [src for src in sources_token.findall('source')]
		sources = []
		
		for source in source_token:
			sources.append({'srcid':source.find('sourceid').text,
					'chanid':taskid,
					'connector':source.find('connector').text,
					'iface':source.find('listenby').text,
					'url':source.find('uri').text,
					'debuglevel':source.find('debuglevel').text,
					'timing':source.find('timing').text,
					'caching':source.find('caching').text,
					'prgid':source.find('pid').text,
					'audioinfo':source.find('audioinfo').text,
					'subtitleinfo':source.find('subtitleinfo').text,
					'volumeoffset':source.find('volumeoffset').text,
					'priority':source.find('priority').text,
					})
		
		for src in sources:
			if src['audioinfo'] == '':
				src['atrack'] = '-1'
			else:
				atrack = src['audioinfo'].split(',')[0]
				try:
					src['atrack'] = int(atrack)
					src['atrack'] = str(atrack)
				except:
					src['atrack'] = '-1'
		#print sources
		print("-----------sout description---------------")
		souts_token = root.findall('encoders')[0]
		sout_token = [src for src in souts_token.findall('encoder')]
		souts = []
		for sout in sout_token:
			item = {}
			for var in sout.getiterator('var'):
				item[var.attrib.get('name').strip()]=var.text.strip()
			if 'mux' not in item:
				access = item['output'][:3]
				if  access == 'udp':
					item['mux'] = 'ts'
				elif access  == 'htt':
					item['mux'] = 'ts'
				elif access  == 'rtm':
					item['mux'] = 'flv'
				elif access  == 'fil':
					#item['mux'] = 'mp4'
					item['mux'] = 'ts'
			souts.append(item)
			del item
		#print souts
		print("-----------formatting for stream engine---------------")
		for src in sources:
			stream={}
			stream[taskid]=(src,[])
			for sout in souts:
				video_param = {}
				audio_param = {}
				subtt_param = {'':''}
				output_param=[]

				print('start ==========================')
				output_param.append({'access_iface':sout['sendby'],'mux':sout['mux'],'access_out':sout['output']})
				if 'vcodec' not in sout and 'acodec' not in sout:
					pass
					#stream[taskid][1].append((video_param,audio_param,output_param))
					#del video_param 
					#del audio_param
					#del output_param
					#continue
				elif 'vcodec' not in sout and 'acodec' in sout:
					audio_param['acodec'] = sout['acodec']
					if audio_param['acodec'] == 'aac':
						audio_param['acodec'] = 'mp4a'
						audio_param['aenc']= 'ffmpeg'
						audio_param['channels']='2'
						audio_param['samplerate'] = '48000'
						audio_param['ab'] = '128'
				elif 'vcodec' in sout and 'acodec' not in sout:
					video_param['vcodec'] = sout['vcodec']
					video_param['venc']='x264'
					video_param['height'] = sout['height']
					video_param['width'] = sout['width']
					video_param['fps'] = sout['frame-num']
					video_param['vb'] = sout['bitrate']
					video_param['threads'] = '24'
					video_param['bframes'] = sout['bframes']
					video_param['profile'] = sout['profile']
					video_param['level'] = sout['level']
					video_param['scenecut'] = sout['dynamic-Iframe']
					video_param['keyint'] = sout['gop']
					video_param['vbv-maxrate'] = sout['maxrate']
					video_param['vbv-bufsize'] = sout['bufsize'] 
					video_param['qpmax'] = sout['video-quality']
					video_param['deinterlace'] = sout['deinterlace']
					video_param['interlace'] = sout['interlace']
					video_param['bitratemode'] = sout['bitratemode']
					video_param['logo-file'] = sout['logofile']
					video_param['x'] = sout['logoxpos']
					video_param['y'] = sout['logoypos']

				else:
					video_param['vcodec'] = sout['vcodec']
					video_param['venc']='x264'
					video_param['height'] = sout['height']
					video_param['width'] = sout['width']
					video_param['fps'] = sout['frame-num']
					video_param['vb'] = sout['bitrate']
					video_param['threads'] = '24'
					video_param['bframes'] = sout['bframes']
					video_param['profile'] = sout['profile']
					video_param['level'] = sout['level']
					video_param['scenecut'] = sout['dynamic-Iframe']
					video_param['keyint'] = sout['gop']
					video_param['vbv-maxrate'] = sout['maxrate']
					video_param['vbv-bufsize'] = sout['bufsize'] 
					video_param['qpmax'] = sout['video-quality']
					video_param['deinterlace'] = sout['deinterlace']
					video_param['interlace'] = sout['interlace']
					video_param['bitratemode'] = sout['bitratemode']
					video_param['logo-file'] = sout['logofile']
					video_param['x'] = sout['logoxpos']
					video_param['y'] = sout['logoypos']

					audio_param['acodec'] = sout['acodec']
					if audio_param['acodec'] == 'aac':
						audio_param['acodec'] = 'mp4a'
						audio_param['aenc']= 'ffmpeg'
						audio_param['channels']='2'
						audio_param['samplerate'] = '48000'
						audio_param['ab'] = '128'
			
				stream[taskid][1].append((video_param,audio_param,subtt_param,output_param))
	
				del video_param 
				del audio_param
				del subtt_param 
				del output_param
			args_list['streams'].append(stream)
			del stream
		"""
		for i in args_list['streams']:
			print i[taskid][0]['priority']
			print i
			print '=========================='
		"""
		print("-----------transcation descripion---------------")
		#action = root.find('action').text
		#if action == 'reset':
		#	args_list['action'] = 'reset'
		#	print "Action: ",args_list['action']
		
		args_list['guid'] = root.find('guid').text
		args_list['status'] = ''
		args_list['resultcode'] ='' 
		args_list['resultmsg'] = ''
		args_list['executetime'] = ''
		args_list['title'] = root.find('title').text
		args_list['state'] = root.find('state').text
		args_list['timeout'] = root.find('timeout').text
		args_list['running'] = 'no'

		print("-----------confirm master---------------")
		order_slave=[]
		for s in args_list['streams']:
			pri = int(s[taskid][0]['priority'])
			if len(args_list['master']) == 0:
				args_list['master'] = s
			elif int(args_list['master'][taskid][0]['priority']) < pri :
				#args_list['slave'].append( args_list['master'] )
				order_slave.append( args_list['master'] )
				args_list['master'] = s
			else:
				#args_list['slave'].append( s )
				order_slave.append( s )
		for s in order_slave:
			pri = int(s[taskid][0]['priority'])
			if len(args_list['slave']) == 0:
				args_list['slave'].append( s )
			elif int(args_list['slave'][0][taskid][0]['priority']) < pri :
				args_list['slave'].insert( 0,s )
			else:
				args_list['slave'].append( s )

		del order_slave
		print('========master==================')
		#print args_list['master'] 
		print('========slave=================')
		#for i in args_list['slave']:
		#	print i
		print('==========================')
	#except:
		#print "Error as parsing xml "
		#error = 1
		return (error,args_list)

"""
def parser_reset(xml):
	args_list = {
		'mode':'none',
		'action':'heartbeat',
		'type':'none',
		}
	return parser_common(args_list,xml)
"""
def parser_cancel(xml):
	args_list = {
		'mode':'none',
		'action':'cancel',
		'type':'none',
		}
	return (0,parser_common(args_list,xml))

def parser_request(xml):
	args_list = {}
	return (0,parser_common(args_list,xml))

def parser_status(xml):
	args_list = {
		'mode':'none',
		'action':'status',
		'type':'none',
		'taskid':'none',
		'resultcode':'none',
		'resultmsg': 'none',
		}
	return (0,parser_common(args_list,xml))

def parser_list(xml):
	args_list = {
		'mode':'none',
		'action':'list',
		'type':'none',
		#'sourceid': 'none',
		}
	return (0,parser_common(args_list,xml))

def parser_analyze(xml):
	args_list = {
		'mode':'none',
		'action':'heartbeat',
		'type':'none',
		'sourceid': 'none',
		}
	return (0,parser_common(args_list,xml))


def body_response_status(args_list):
	if args_list['request'] > 0:
		args_list['resultcode'] = str(args_list['request'])
		args_list['resultmsg'] = 'no found'
	return body_response(args_list)

def body_response_request(args_list):
	if 'org_request' in args_list:
		return args_list['org_request']
	else:
		args_list['resultcode'] = '404'
		args_list['resultmsg'] = 'no found'
	return body_response(args_list)

def body_response(args_list):
	if 'request' in args_list:
		del args_list['request']
	root=ET.Element('result')
	for item in args_list:
		sub = ET.SubElement(root, item)
		sub.text = args_list[item]
	print(ET.tostring(root))
	return ET.tostring(root)
	
def body_response_list(args_list):
	print('RETURN',args_list)
	root=ET.Element('result')
	if 'request' in args_list:
		del args_list['request']
	for item in args_list:
		if item != 'return':
			sub = ET.SubElement(root, item)
			sub.text = args_list[item]
	if 'return' in args_list:
		tasks = ET.SubElement(root, 'tasks')
		for t,s in args_list['return']:
			task = ET.SubElement(tasks, 'taskid')
			task.text = t
			task.attrib = {'state':s}
	
	return ET.tostring(root)

def build_notify_body(args_list):
	root=ET.Element('notify')
	for item in args_list:
		if item == 'slave_index' or item == 'streams' or item == 'master' or item == 'slave' or item == 'timeout' or item == 'timeout_left' or item == 'request' or item == 'exception' or item == 'can_switch' or item == 'switch' or item == 'server_sync' or item == 'running' or item == 'replyaddr' or item == 'notifyaddr' or item == 'version' or item == 'status'or item == 'org_request' or item == 'executetime' or item == 'switch_next' or item == 'pid' or item == 'task' or item == 'nodeid' or item == 'source':
			continue
		sub = ET.SubElement(root, item)
		sub.text = args_list[item]
	taskid = list(args_list['master'])[0]
	#tasks_tuple = args_list['master'][taskid]
	#task = ET.SubElement(root, 'task')
	#for t in tasks_tuple[0]
	#	if t == 'ch' or
	t = ET.SubElement(root, 'taskid')
	t.text = taskid 
	#t = ET.SubElement(task, 'pid')
	#t.text = tasks_tuple[0]['prgid']

	return ET.tostring(root)

def build_request_body(args_list):
	root=ET.Element('result')
	for item in args_list:
		if item == 'slave_index' or item == 'streams' or item == 'master' or item == 'slave' or item == 'timeout' or item == 'timeout_left' or item == 'request' or item == 'exception' or item == 'can_switch' or item == 'switch' or item == 'server_sync' or item == 'running' or item == 'replyaddr' or item == 'notifyaddr' or item == 'version' or item == 'status' or item == 'org_request' or item == 'srcid' or item == 'switch_next' or item == 'source':
			continue
		sub = ET.SubElement(root, item)
		sub.text = args_list[item]
	taskid = list(args_list['master'])[0]
	tasks_tuple = args_list['master'][taskid]
	task = ET.SubElement(root, 'task')
	#for t in tasks_tuple[0]
	#	if t == 'ch' or
	t = ET.SubElement(task, 'taskid')
	t.text = taskid 
	t = ET.SubElement(task, 'pid')
	t.text = tasks_tuple[0]['prgid']

	return ET.tostring(root)
	
def build_body(cmd):
	body=''
	if 'request' in cmd[0] and cmd[0]['request'] == -1:
		print('transcation is error')
		body =	response_body_common % cmd[0]['request'] 

	if cmd[0]['action'] == 'heartbeat':
		body = body_response(cmd[0]);
	elif cmd[0]['action'] == 'encode':
		#body = body_response(cmd[0]);
		body =	response_body_common % cmd[0]['request'] 
	elif cmd[0]['action'] == 'reset':
		#body = body_response(cmd[0]);
		body =	response_body_common % cmd[0]['request'] 
	elif cmd[0]['action'] == 'cancel':
		body =	response_body_common % cmd[0]['request'] 
	elif cmd[0]['action'] == 'stop':
		body =	response_body_common % cmd[0]['request'] 
	elif cmd[0]['action'] == 'start':
		body =	response_body_common % cmd[0]['request'] 
	elif cmd[0]['action'] == 'status':
		body = body_response_status(cmd[0]);
	elif cmd[0]['action'] == 'request':
		body = body_response_request(cmd[0]);
	elif cmd[0]['action'] == 'list':
		body = body_response_list(cmd[0]);
	elif cmd[0]['action'] == 'analyze':
		body = body_response(cmd[0]);
	elif cmd[0]['action'] == 'config':
		body = body_response(cmd[0]);
	else:
		print('action is empty')
		body =	response_body_common % cmd[0]['request'] 
		pass

	return body

def parser(xml):
	error = 0
	args_list={}
	try:
		et = ET.fromstring(xml)
		#et = ET.parse(io.StringIO(xml))
		action = et.find('action').text
		#print action
		if action == 'heartbeat':
			error ,args_list = parser_heartbeat(xml)	
			#return response_body_common
		elif action == 'encode':
			error ,args_list = parser_encode(xml,'encode')	
		elif action == 'reset':
			error ,args_list = parser_encode(xml,'reset')	
			#args_list = parser_reset(xml)	
		elif action == 'cancel':
			error ,args_list = parser_cancel(xml)	
		elif action == 'start':
			error ,args_list = parser_cancel(xml)	
		elif action == 'stop':
			error ,args_list = parser_cancel(xml)	
		elif action == 'status':
			error ,args_list = parser_status(xml)	
		elif action == 'request':
			error ,args_list = parser_request(xml)	
		elif action == 'list':
			error ,args_list = parser_list(xml)	
		elif action == 'analyze':
			error ,args_list = parser_analyze(xml)	
		elif action == 'config':
			error ,args_list = _parser_config(xml)	
		else:
			args_list['action']='empty'
			print('action is empty')
			error = 1;
	except:
		print("Error as parsing xml ")
		error = 1

	sem=threading.Semaphore(0)
	return (args_list, sem, error)

def application(env, start_response):
	body=''
	input = env['wsgi.input']
	method = env['REQUEST_METHOD']

	if method == 'POST':
		content = input.read(int(env.get('CONTENT_LENGTH','0')))
		#print content
		context = parser(content)
		if context[2] == 0 :
			env['queue'].put(context)
			#print 'enqueue'
			context[1].acquire()
			#print 'dequeue'
			body = build_body(context)
		else:
			body='<response><status>101</status><message>XmlError()</message></response>'

	if method == 'GET':
		content = input.read(int(env.get('CONTENT_LENGTH','0')))
		#print content
	#if env['PATH_INFO'] != '/poster':
	#	start_response('404 Not Found', [('Content-Type', 'text/plain')])
	#	return ['Not Found\r\n']
	start_response('200 OK', [('Content-Type', 'text/plain')])
	#pdb.set_trace()
	#env['queue'].put()
	#print env['count'] 
	return [xml_header+body+'\r\n']
	#return ['Yes, I know. please wait!\r\n']

def start_wsgi(listen, app, queue):
	wsgi.server(listen, app,environ={"queue": queue})

def wsgi_init(port=8091):
	task_queue = Queue()
	socket = eventlet.listen(('', port))
	_thread.start_new_thread(start_wsgi, (socket, application,task_queue))
	return (task_queue,socket)
	
EventTick = 0.3
def wsgi_get_event(task_queue):
	command=()
	try:
		command=task_queue.get(True,0.01)
		return command
	except:
		#print "Queue is empty"	
		pass

def server_sync_thread(trans,default):
	headers = {"Content-type": "text/xml", "Accept": "text/xml"}
	#print trans
	while 1:
		try:
			url = trans['replyaddr'].split('/')
			#print url
			address=[]
			if ':' in url[2]:
				address= url[2].split(':')
			else:
				address=[url[2],'80']
			#print address[0],address[1]
			httpClient = httplib.HTTPConnection(address[0], int(address[1]), timeout=30)
			#print "****", httpClient
			del url[0]
			del url[0]
			del url[0]
			
			url_string=''
			for u in url:
				url_string += '/' + u
			#print url_string
			trans['requestid'] = trans['transid']
			del trans['transid']
			trans['executetime']=time.strftime('%Y-%m-%d %H:%M:%S',time.localtime())
			if trans['exception'] > 2 * trans['slave_index'][1]:
				trans['resultcode'] = '201'
				trans['resultmsg'] = 'failed'
			else:
				trans['resultcode'] = '0'
				trans['resultmsg'] = 'successful'

			response_body = build_request_body(trans)
			body = xml_header+ response_body +'\r\n'
			#print "Result:",body
			httpClient.request("POST", url_string, body, headers)
			response = httpClient.getresponse()
			response.read()
			if trans['resultcode'] != '0':
				trans['state'] = 'cancel'
			trans['server_sync'] = 'yes'
			httpClient.close()
			#print "Result Close this connection ************ "
			break
		except Exception as e:
			#print "ResultNotify ",e
			time.sleep(3)

def server_async_notify(trans,args,errno):
	headers = {"Content-type": "text/xml", "Accept": "text/xml"}
	#print trans
	try:
		url = trans['notifyaddr'].split('/')
		#print url
		address=[]
		if ':' in url[2]:
			address= url[2].split(':')
		else:
			address=[url[2],'80']
		#print address[0],address[1]
		httpClient = httplib.HTTPConnection(address[0], int(address[1]), timeout=30)
		#print "****", httpClient
		del url[0]
		del url[0]
		del url[0]
		
		url_string=''
		for u in url:
			url_string += '/' + u
		#print url_string
		#trans['requestid'] = trans['transid']
		trans['resultcode'] = str(errno)
		if errno == 2001:
			trans['resultmsg'] = 'Lost'
		elif errno == 2002:
			trans['resultmsg'] = 'Reclock'
		elif errno == 2003:
			trans['resultmsg'] = 'Restart for timing alarm'
		elif errno == 2000:
			if args == 101:
				trans['resultmsg'] = 'Restart for timing alarm'
				trans['resultcode'] = str(2101)
			elif args == 198:
				trans['resultmsg'] = 'Restart for audio sync'
				trans['resultcode'] = str(2198)
			elif args == 252:
				trans['resultmsg'] = 'Restart for only one track'
				trans['resultcode'] = str(2252)
			elif args == 253:
				trans['resultmsg'] = 'Restart for muxing failed by transcoding exception'
				trans['resultcode'] = str(2253)
			elif args == 199:
				trans['resultmsg'] = 'Restart for source error'
				trans['resultcode'] = str(2199)
			elif args == 197:
				trans['resultmsg'] = 'Restart for no source available'
				trans['resultcode'] = str(2197)
			elif args == 254:
				trans['resultmsg'] = 'Restart for no output by muxing error'
				trans['resultcode'] = str(2254)
			else: 
				trans['resultmsg'] = 'Restart for unknow error'
		elif errno == 2004:
			trans['srcid'] = args.values()[0][0]['srcid']
			trans['resultmsg'] = 'Switch %s' % args.values()[0][0]['url']

		
		response_body = build_notify_body(trans)
		body = xml_header+ response_body +'\r\n'
		#print "Notify:", body
		httpClient.request("POST", url_string, body, headers)
		response = httpClient.getresponse()
		response.read()
		httpClient.close()
		#print "Notify Close this connection ************ "
	except Exception as e:
		print("NotifyResult",e)

def server_async_thread(trans,default):
	headers = {"Content-type": "text/xml", "Accept": "text/xml"}
	#print trans
	try:
		url = trans['notifyaddr'].split('/')
		#print url
		address=[]
		if ':' in url[2]:
			address= url[2].split(':')
		else:
			address=[url[2],'80']
		#print address[0],address[1]
		httpClient = httplib.HTTPConnection(address[0], int(address[1]), timeout=30)
		#print "****", httpClient
		del url[0]
		del url[0]
		del url[0]
		
		url_string=''
		for u in url:
			url_string += '/' + u
		#print url_string
		trans['requestid'] = trans['transid']
		trans['resultcode'] = '-1'
		
		response_body = build_notify_body(trans)
		body = xml_header+ response_body +'\r\n'
		#print "Response:", body
		httpClient.request("POST", url_string, body, headers)
		response = httpClient.getresponse()
		response.read()
		httpClient.close()
		print("Response Close this connection ************ ")
	except Exception as e:
		print("ResponseResult:",e)



def wsgi_main():
	task_queue = Queue()
	_thread.start_new_thread(start_wsgi, (eventlet.listen(('', 8090)),application,task_queue))
	command=() 
	while 1:
		try:
			command = task_queue.get(0)
			#print "get",command[0]['action']
			transcation_process(command)
			#command[1].release()
		except:
			#print "Queue is empty"	
			pass
		time.sleep(1)

if __name__ == '__main__':
	#wsgi_main()
	#parser_encode('example.xml')
	parser_config('/etc/itvencoder/itvencoder.xml')

"""
**************************************************************************************
def copy_what(count):
	#print "copy what ",count
	count.append(1)

root=ET.Element('goods')
name_con=['yhb','lwy']
size_con=['175','170']
for i in range(2):
#   skirt=ET.SubElement(root,'skirt')
#   skirt.attrib['index']=('%s' %i)
    skirt=ET.SubElement(root,'skirt',index=('%s' %i))
    name=ET.SubElement(skirt,'name')  #
    name.text=name_con[i]             #
    size=ET.SubElement(skirt,'size')
    size.text=size_con[i]
    tree=ET.ElementTree(root)
ET.dump(tree)


import httplib, urllib

httpClient = None
try:
    body = urllib.urlencode({'name': 'tom', 'age': 22})
    headers = {"Content-type": "application/x-www-form-urlencoded"
                    , "Accept": "text/plain"}

    httpClient = httplib.HTTPConnection("localhost", 80, timeout=30)
    httpClient.request("POST", "/test.php", body, headers)
    #httpClient.request("GET", "/test.php")

    response = httpClient.getresponse()
    print response.status
    print response.reason
    print response.read()
    print response.getheaders() #
except Exception, e:
    print e
finally:
    if httpClient:
        httpClient.close()
"""

