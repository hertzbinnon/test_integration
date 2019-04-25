import engineer#.Engineer as Engineer
import copy
import binding
import sys,os

class EngineVLC(engineer.Engineer):
	"""
		Object with certain arch class 
	"""
	def __init__(self,usage):
		engineer.Engineer.__init__(self,'VLC', usage)
		self['arch'] = binding
		self['id']=usage[0]['chanid']
		self['rootdir']=usage[0]['rootdir']
		os.environ['VLC_PLUGIN_PATH']=self['rootdir']+'/modules'
		#import pdb
		#pdb.set_trace()
		if usage[0]['url'][:3] == 'udp' and usage[0]['url'][6] != '@':
			usage[0]['url'] = 'udp://@' + usage[0]['url'][6:]
		self['url'] = usage[0]['url']
		self['args']  =  self.__parsingUsage(self.usage)
		self['inst']   = None#self['arch'].Instance(self['args'])
		self['mediaplayer'] = None #self['inst'].media_player_new()
		self['eventmanager'] = None #self['inst'].media_player_new()
		self['get_state'] = self['arch'].libvlc_media_player_get_state
		print('===>',self['args'])
		
	def shutdown(self):
		if self['inst']: 
			self['inst'].release()
			self['inst'] = None
		if self['mediaplayer']: 
			self['mediaplayer'].release()
			self['mediaplayer'] = None

	def start(self):
		self['inst']   = self['arch'].Instance(self['args'])
		self['mediaplayer'] = self['inst'].media_player_new()
		self['media']  = self['inst'].media_new(self['url'])
		self['mediaplayer'].set_media(self['media'])
		self['eventmanager'] = self['mediaplayer'].event_manager()
		
		return self['mediaplayer'].play()

	def stop(self):
		return self['mediaplayer'].stop()

	def set_event(self,state,func,args):
		if state == 2001:
			self['eventmanager'].event_attach(self['arch'].EventType.MediaPlayerEncounteredError_I,func,args);
		elif state == 2002:
			self['eventmanager'].event_attach(self['arch'].EventType.MediaPlayerEncounteredError_N,func,args);
		elif state == 2003:
			self['eventmanager'].event_attach(self['arch'].EventType.MediaPlayerEncounteredError_T,func,args);

	def checking_status(self):
		return self['get_state'](self['mediaplayer'])

	def __parsingUsage(self, usage):
		if not isinstance(usage,tuple):
			print("EngineVLC() args must be tuple")
			return ''
		args = ' -vvv --ttl 128 --no-avcodec-hurry-up --avcodec-skip-frame -1 --avcodec-threads 8 --file-logging --logmod text '
		#log process
		logpath_str=''
		log_root = ''
		log_format =  '/itv-channel-' + usage[0]['chanid']
		if 'logpath' in usage and usage['logpath']:
			log_root = usage['logpath']+ log_format
			logpath_str = log_root  + '/smsd.log'
		else:
			log_root = self['rootdir'] +'/log'+log_format
			logpath_str = log_root+ '/smsd.log'
		try:
			os.makedirs(log_root)
		except Exception as e:
			print('Log direcory already exist', e)

		log_level = '0'
		if 'debuglevel' in usage[0] and usage[0]['debuglevel'] != None:
			log_level = usage[0]['debuglevel']
		else:
			usage[0]['debuglevel'] = log_level 
		caching = '500'
		if 'caching' in usage[0] and usage[0]['caching'] != None:
			caching= usage[0]['caching']
		else:
			usage[0]['caching'] = caching
			
			
		args += ' --logfile ' + logpath_str
		args += ' --log-verbose ' + log_level#usage['app']['debuglevel']
		args += ' --network-caching ' + caching#usage['app']['caching']
		args += ' --ts-prog-location ' + log_root + '/program.list'

		args += ' ' 		 + usage[0]['url']	
		if usage[0]['iface'] != '':
			args += ' --miface ' + usage[0]['iface']
		if usage[0]['prgid']!= '-1':#and usage[0]['atrack'] == '-1':
			#args += ' --no-sout-all  --program '+ usage[0]['prgid']
			args += ' --program '+ usage[0]['prgid']
		
		args += ' ' +  self.__pipelineUsage(usage[1],(usage[0]['prgid'],usage[0]['atrack'],usage[0]['strack'],usage[0]['soverlay']))
		return args

	def __subpipelineUsage(self,subusage,sformat,soverlay):
			format = ''
			subformat = ''
			ts_mux='ts'
			filter=''
			overlay=''
			video = subusage[0]
			audio = subusage[1]
			subtitle=subusage[2]
			output=subusage[3]
			l = len(output)	
			if len(video) <= 2 and  len(audio) <= 2: #
				if l > 1:
					#print 'debuging 1'
					for out in output:
						#format+= 'dst=\'std{access=%s,mux=%s,dst=%s\',select=\'program=%s,es=%s\',,'
						#format = format % (out['access'],'ts',out['dst'],'-1','-1')
						access_out  = out['access_out'][:4]
						if  access_out[:3] == 'udp':
							format+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
										('udp{miface-addr=%s}' % out['access_iface'],'ts',out['access_out'][6:]) + sformat
						elif access_out == 'http':
							format+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
										('http' ,'ts',out['access_out'][7:]) + sformat
						elif access_out == 'file':
							format+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
										('file' ,'ts',out['access_out'][7:]) + sformat
						else:
							print('Access out not support')
							exit()
							
					format= 'duplicate{' + format + '}'
					#print format 
				else:
					#print 'debuging 2'
					access_out  = output[0]['access_out'][:4]
					if access_out[:3] == 'udp':
						format += 'std{access=%s,mux=%s,dst=%s}' % \
							('udp{miface-addr=%s}' % output[0]['access_iface'] ,'ts',output[0]['access_out'][6:])
					elif acdess_out == 'http':
						format += 'std{access=%s,mux=%s,dst=%s}' % \
							('http' ,'ts',output[0]['access_out'][7:])
					elif acdess_out == 'file':
						format += 'std{access=%s,mux=%s,dst=%s}' % \
							('file' ,'ts',output[0]['access_out'][7:])
					else:
						print('Access out not support')
						exit()
						
					if sformat != '':
						format= 'duplicate{dst=\'' + format + '\','+ sformat + '}'
					#print format 
			elif len(video) <= 2 or len(audio) < 2: #
				if len(video) <= 2 :
					format = 'acodec=%s,ab=128,channels=%s,samplerate=%s,aenc=%s'
					format = format % ( audio['acodec'],audio['channels'],audio['samplerate'],audio['aenc'])
					format = 'transcode{' + format + '}'
					if l > 1:	
						for out in output:
							#subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s\',select=\'program=%s,es=%s\',,'
							#subformat = subformat % (out['access'],'ts',out['dst'],'-1','-1')
							access_out  = out['access_out'][:4]
							if access_out[:3] != 'udp':
								subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
										('udp{miface-addr=%s}' % out['access_iface'],ts_mux,out['access_out'][6:])
							elif access_out == 'http':
								subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
										('http' ,ts_mux, out['access_out'][7:])
							elif access_out == 'file':
								subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
										('file' ,ts_mux, out['access_out'][7:])
							else:
								print('Access out not support')
								exit()

						subformat= 'duplicate{' + subformat + '}'
						format += ':'+subformat
						if sformat != '':
							format= 'duplicate{dst=\'' + format + '\','+ sformat + '}'
					else:
							#print 'debuging 5'
						access_out  = output[0]['access_out'][:4]
						if access_out[:3] == 'udp':
							subformat += 'std{access=%s,mux=%s,dst=%s}' % \
								('udp{miface-addr=%s}' % output[0]['access_iface'] ,ts_mux,output[0]['access_out'][6:])
						elif access_out == 'http':
							subformat += 'std{access=%s,mux=%s,dst=%s}' % \
								('http' ,ts_mux, output[0]['access_out'][7:])
						elif access_out == 'file':
							subformat += 'std{access=%s,mux=%s,dst=%s}' % \
								('file' ,ts_mux, output[0]['access_out'][7:])
						else:
							print('Access out not support')
							exit()
						format += ':'+subformat 
				else:
					# TO DO: audio is none
					pass
			else:
				#print 'debuging 3'
				if soverlay == 'overlay':
					overlay = 'soverlay'
				if video['bitratemode'] == '1':
					ts_mux='ts{shaping=1000,bmax=0,bmin=%s}' % str( int( int(video['vb']) * 0.30 * 1000) ) 
					video['bframes'] = '1'
				if video['logo-file'] !='':
					filter=',sfilter=logo{file=%s,x=%s,y=%s},' % (video['logo-file'],video['x'],video['y'])
				if video['deinterlace'] == '1':
					format+= 'vcodec=%s,fps=%s,deinterlace=1,deinterlace-module=deinterlace{mode=blend},width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,%s},'
				elif video['interlace'] == '1':
					video['width']='0'
					video['height']='0'
					format+= 'vcodec=%s,fps=%s,width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,interlaced,weightp=0,%s},no-deinterlace,'
				elif video['interlace'] == '2':
					video['width']='0'
					video['height']='0'
					format+= 'vcodec=%s,fps=%s,width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,fake-interlaced,%s},no-deinterlace,'
				else:
					format+= 'vcodec=%s,fps=%s,width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,%s},no-deinterlace,'
				#format+= 'vcodec=%s,fps=%s,deinterlace=1,deinterlace-module=deinterlace{mode=blend},width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,profile=high,level=4.1,ref=3,bpyramid=strict},'
				from multiprocessing import cpu_count
				threads = cpu_count()
				qpmax=''
				if video['qpmax'] == '4':
					qpmax = '36'
				elif video['qpmax']== '3': 
					qpmax = '41'
				elif video['qpmax']== '2': 
					qpmax = '46'
				elif video['qpmax']== '1': 
					qpmax = '51'
				elif video['qpmax']== 'cbr-ex': 
					qpmax = '37'
					video['vb']=str( int(int(video['vb']) / 0.90)  )
					video['vbv-maxrate']=video['vb']
					video['vbv-bufsize']=str(int(int(video['vb'])/int(video['fps'])))
					video['bframes']= '0'
				else:
					qpmax = '32'
				#video['threads'] = str(threads)
				if video['threads'] == '0':
					video['threads']=str(threads/2+threads/4+threads/8)
				preset = ''
				if video['venc'] == 'x265':
					preset=''#'preset=ultrafast,tune=zerolatency'
				format = format % (video['vcodec'],video['fps'],video['width'],video['height'],\
											video['threads'],video['vb'],video['venc'],video['vbv-maxrate'],\
											video['vbv-bufsize'],video['keyint'],qpmax,video['scenecut'],	\
											video['bframes'],preset)
				format += 'acodec=%s,ab=128,channels=%s,samplerate=%s,aenc=%s'
				format = format % ( audio['acodec'],audio['channels'],audio['samplerate'],audio['aenc'])
				format += filter
				format += overlay 
				format = 'transcode{' + format + '}'
				if l > 1:
					#print 'debuging 4'
					for out in output:
						#subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s\',select=\'program=%s,es=%s\',,'
						#subformat = subformat % (out['access'],'ts',out['dst'],'-1','-1')
						access_out  = out['access_out'][:4]
						if access_out[:3] == 'udp':
							subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
									('udp{miface-addr=%s}' % out['access_iface'],ts_mux,out['access_out'][6:])
						elif access_out == 'http':
							subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
									('http' ,ts_mux, out['access_out'][7:])
						elif access_out == 'file':
							subformat+= 'dst=\'std{access=%s,mux=%s,dst=%s}\',,' % \
									('file' ,ts_mux, out['access_out'][7:])
						else:
							print('Access out not support')
							exit()
					subformat= 'duplicate{' + subformat + '}'
					format += ':'+subformat
					if sformat != '':
						format= 'duplicate{dst=\'' + format + '\','+ sformat + '}'
					#print format 
				else:		
					#print 'debuging 5'
					access_out  = output[0]['access_out'][:4]
					if access_out[:3] == 'udp':
						subformat += 'std{access=%s,mux=%s,dst=%s}' % \
							('udp{miface-addr=%s}' % output[0]['access_iface'] ,ts_mux,output[0]['access_out'][6:])
					elif access_out == 'http':
						subformat += 'std{access=%s,mux=%s,dst=%s}' % \
							('http' ,ts_mux, output[0]['access_out'][7:])
					elif access_out == 'file':
						subformat += 'std{access=%s,mux=%s,dst=%s}' % \
							('file' ,ts_mux, output[0]['access_out'][7:])
					else:
						print('Access out not support')
						exit()
						
					format += ':'+subformat 
					#print format 
			return format

	def __pipelineUsage(self,usage,select):
		"""usage:[({video}, {audio}, [out]), (), ()], select:(prgid,audiotrack)"""
		prgid = select[0]
		atrack = select[1]
		strack = select[2]
		soverlay = select[3]
		sformat = ''
		sf=''
		af=''
		vf=''
		if prgid != '-1' :
			sformat = 'select=\'program='+prgid+',%s,%s\''
			if atrack != '-1':
				af = 'es='+atrack
			if strack != '-1':
				sf = 'es='+strack
			sformat = sformat % (af,sf)
		
		if not isinstance(usage, list):
			print("pipeline args must be list")
		#print '---',usage
		if len(usage) > 1:
			#print 'debuging -1'
			format=''
			for out in usage:
				#format+='dst=\''+self.__subpipelineUsage(out)+'\',select=\'program=%s,es=%s\',,'
				format+='dst=\''+self.__subpipelineUsage(out,sformat,soverlay)+'\',,' + sformat
			format = 'duplicate{'+ format +'}'
			#print format 
		else : # raw output [({}, {}, [{out},{out}]), (), ()]
			print('debuging 0')
			format = self.__subpipelineUsage(usage[0],sformat,soverlay)
			if sformat != '':
				format= 'duplicate{dst=\'' + format + '\','+ sformat + '}'
			#print format 
		#print 'format +++',format
		return "--sout=#%s" % format
def event_Func(event,trans):
	errno = -1 
	if event.type == binding.EventType.MediaPlayerEncounteredError_I:
		errno = 2001
	elif event.type == binding.EventType.MediaPlayerEncounteredError_N:
		errno = 2002
	elif event.type == binding.EventType.MediaPlayerEncounteredError_T:
		errno = 2003
	print("yyyyyyyyy")

if  __name__ == '__main__':
	import os,sys
	import binding
	import wsgi
	argv = sys.argv
	if len(argv) < 2:
		exit()
	#os.environ['VLC_PLUGIN_PATH']='/home/smsd/modules'
	#os.environ['LD_LIBRARY_PATH']='/home/smsd/modules:/home/hebin/.temp/cerbero/build/dist/linux_x86_64/lib:/home/hebin/.temp/cerbero/deps/installed/lib:/home/hebin/.temp/Gcc/installed/lib64'
	#print os.environ['LD_LIBRARY_PATH']
	streamer = StreamingMedia()
	config_file = '/etc/itvencoder/itvencoder.xml'
	#print("=====>")
	args_lists=wsgi.parser_config(config_file)
	#print("=====>",args_lists)
	streamer.add_engines(args_lists[0]['master'],'VLC')
	print([i for i in streamer.engine])
	id = argv[1]
	streamer.start(id)
	#streamer.set_event(id,2001,event_Func,None)
	import time
	while 1:
		#print(streamer.check_status(id))
		time.sleep(1.0)	
