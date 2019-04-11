import engineer#.Engineer as Engineer
import copy
import binding

class EngineVLC(Engineer):
	"""
		Object with certain arch class 
	"""
	def __init__(self,usage):
		Engineer.__init__(self,'VLC', usage)
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
			import sys,os
			log_root = sys.argv[1]+'/log'+log_format
			logpath_str = log_root+ '/smsd.log'
		try:
			os.mkdir(log_root)
		except:
			print('File exist', log_root)

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
		if usage[0]['prgid']!= '-1' and usage[0]['atrack'] == '-1':
			args += ' --no-sout-all  --program '+ usage[0]['prgid']
		
		args += ' ' +  self.__pipelineUsage(usage[1],(usage[0]['prgid'],usage[0]['atrack']))
		return args

	def __subpipelineUsage(self,subusage,sformat):
			format = ''
			subformat = ''
			ts_mux='ts'
			filter=''
			video = subusage[0]
			audio = subusage[1]
			output=subusage[2]
			l = len(subusage[2])	
			#if len(subusage[0]) == 0 and  len(subusage[1]) == 0: #
			if len(subusage[0]) <= 2 and  len(subusage[1]) <= 2: #
				if l > 1:
					#print 'debuging 1'
					for out in output:
						#format+= 'access_out=\'std{access=%s,mux=%s,access_out=%s\',select=\'program=%s,es=%s\',,'
						#format = format % (out['access'],'ts',out['access_out'],'-1','-1')
						if out['access_out'][:4] != 'http':
							format+= 'access_out=\'std{access=%s,mux=%s,access_out=%s}\',,' % \
										('udp{miface-addr=%s}' % out['access_iface'],'ts',out['access_out'][6:]) + sformat
						else:
							format+= 'access_out=\'std{access=%s,mux=%s,access_out=%s}\',,' % \
										('http' ,'ts',out['access_out'][7:]) + sformat
							
					format= 'duplicate{' + format + '}'
					#print format 
				else:
					#print 'debuging 2'
					if output[0]['access_out'][:4] != 'http':
						format += 'std{access=%s,mux=%s,access_out=%s}' % \
							('udp{miface-addr=%s}' % output[0]['access_iface'] ,'ts',output[0]['access_out'][6:])
					else:
						format += 'std{access=%s,mux=%s,access_out=%s}' % \
							('http' ,'ts',output[0]['access_out'][7:])
					if sformat != '':
						format= 'duplicate{access_out=\'' + format + '\','+ sformat + '}'
					#print format 
			elif len(video) <= 2 or len(audio) < 2: #
				if len(video) <= 2 :
					format = 'acodec=%s,ab=128,channels=%s,samplerate=%s,aenc=%s'
					format = format % ( audio['acodec'],audio['channels'],audio['samplerate'],audio['aenc'])
					format = 'transcode{' + format + '}'
					if l > 1:	
						for out in output:
							#subformat+= 'access_out=\'std{access=%s,mux=%s,access_out=%s\',select=\'program=%s,es=%s\',,'
							#subformat = subformat % (out['access'],'ts',out['access_out'],'-1','-1')
							if out['access_out'][:4] != 'http':
								subformat+= 'access_out=\'std{access=%s,mux=%s,access_out=%s}\',,' % \
										('udp{miface-addr=%s}' % out['access_iface'],ts_mux,out['access_out'][6:])
							else:
								subformat+= 'access_out=\'std{access=%s,mux=%s,access_out=%s}\',,' % \
										('http' ,ts_mux, out['access_out'][7:])
						subformat= 'duplicate{' + subformat + '}'
						format += ':'+subformat
						if sformat != '':
							format= 'duplicate{access_out=\'' + format + '\','+ sformat + '}'
					else:
							#print 'debuging 5'
						if output[0]['access_out'][:4] != 'http':
							subformat += 'std{access=%s,mux=%s,access_out=%s}' % \
								('udp{miface-addr=%s}' % output[0]['access_iface'] ,ts_mux,output[0]['access_out'][6:])
						else:
							subformat += 'std{access=%s,mux=%s,access_out=%s}' % \
								('http' ,ts_mux, output[0]['access_out'][7:])
						format += ':'+subformat 
				else:
					# TO DO: audio is none
					pass
			else:
				#print 'debuging 3'
				if video['bitratemode'] == '1':
					ts_mux='ts{shaping=1000,bmax=0,bmin=%s}' % str( int( int(video['vb']) * 0.30 * 1000) ) 
					video['bframes'] = '1'
				if video['logo-file'] !='':
					filter=',sfilter=logo{file=%s,x=%s,y=%s},' % (video['logo-file'],video['x'],video['y'])
				if video['deinterlace'] == '1':
					format+= 'vcodec=%s,fps=%s,deinterlace=1,deinterlace-module=deinterlace{mode=blend},width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s},'
				elif video['interlace'] == '1':
					video['width']='0'
					video['height']='0'
					format+= 'vcodec=%s,fps=%s,width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,interlaced,weightp=0},no-deinterlace,'
				elif video['interlace'] == '2':
					video['width']='0'
					video['height']='0'
					format+= 'vcodec=%s,fps=%s,width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s,fake-interlaced},no-deinterlace,'
				else:
					format+= 'vcodec=%s,fps=%s,width=%s,height=%s,threads=%s,vb=%s,venc=%s{vbv-maxrate=%s,vbv-bufsize=%s,keyint=%s,min-keyint=25,qpmax=%s,scenecut=%s,bframes=%s},no-deinterlace,'
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
					video['vbv-bufsize']=str(int(video['vb'])/int(video['fps']))+',tune=zerolatency'
				else:
					qpmax = '32'
				#video['threads'] = str(threads)
				if video['threads'] == '0':
					video['threads']=str(threads/2+threads/4+threads/8)
				format = format % (video['vcodec'],video['fps'],video['width'],video['height'],\
											video['threads'],video['vb'],video['venc'],video['vbv-maxrate'],\
											video['vbv-bufsize'],video['keyint'],qpmax,video['scenecut'],	\
											video['bframes'])
				format += 'acodec=%s,ab=128,channels=%s,samplerate=%s,aenc=%s'
				format = format % ( audio['acodec'],audio['channels'],audio['samplerate'],audio['aenc'])
				format += filter
				format = 'transcode{' + format + '}'
				if l > 1:
					#print 'debuging 4'
					for out in output:
						#subformat+= 'access_out=\'std{access=%s,mux=%s,access_out=%s\',select=\'program=%s,es=%s\',,'
						#subformat = subformat % (out['access'],'ts',out['access_out'],'-1','-1')
						if out['access_out'][:4] != 'http':
							subformat+= 'access_out=\'std{access=%s,mux=%s,access_out=%s}\',,' % \
									('udp{miface-addr=%s}' % out['access_iface'],ts_mux,out['access_out'][6:])
						else:
							subformat+= 'access_out=\'std{access=%s,mux=%s,access_out=%s}\',,' % \
									('http' ,ts_mux, out['access_out'][7:])
					subformat= 'duplicate{' + subformat + '}'
					format += ':'+subformat
					if sformat != '':
						format= 'duplicate{access_out=\'' + format + '\','+ sformat + '}'
					#print format 
				else:		
					#print 'debuging 5'
					if output[0]['access_out'][:4] != 'http':
						subformat += 'std{access=%s,mux=%s,access_out=%s}' % \
							('udp{miface-addr=%s}' % output[0]['access_iface'] ,ts_mux,output[0]['access_out'][6:])
					else:
						subformat += 'std{access=%s,mux=%s,access_out=%s}' % \
							('http' ,ts_mux, output[0]['access_out'][7:])
					format += ':'+subformat 
					#print format 
			return format

	def __pipelineUsage(self,usage,select):
		"""usage:[({video}, {audio}, [out]), (), ()], select:(prgid,audiotrack)"""
		prgid = select[0]
		atrack = select[1]
		sformat = ''
		if prgid != '-1' and atrack == '-1':
			sformat = 'select=\'program=%s\',' % prgid
		elif prgid != '-1' and atrack != '-1':
			sformat = 'select=\'program=%s,es=%s\',' % (prgid,atrack)
		if not isinstance(usage, list):
			print("pipeline args must be list")
		#print '---',usage
		if len(usage) > 1:
			#print 'debuging -1'
			format=''
			for out in usage:
				#format+='access_out=\''+self.__subpipelineUsage(out)+'\',select=\'program=%s,es=%s\',,'
				format+='access_out=\''+self.__subpipelineUsage(out,sformat)+'\',,' + sformat
			format = 'duplicate{'+ format +'}'
			#print format 
		else : # raw output [({}, {}, [{out},{out}]), (), ()]
			print('debuging 0')
			format = self.__subpipelineUsage(usage[0],sformat)
			if sformat != '':
				format= 'duplicate{access_out=\'' + format + '\','+ sformat + '}'
			#print format 
		#print 'format +++',format
		return "--sout=#%s" % format


class StreamingMedia(): 
	""" 
		Focus on statistic data, status, control about a stream object
	"""
	__archclass__ = {'GST':EngineGST,'FFM':EngineFFM,'VLC':EngineVLC}
	def __init__(self,usage_config=None,arch ='VLC'):
		if not usage_config:
			self.usage_params = {}
		else:
			self.usage_params = StreamingMedia.usage_param(usage_config)
		#for u in self.usage_params:
		#	print self.usage_params[u]
		self.engine = {}
		
		for u in self.usage_params:
			self.engine[u] = StreamingMedia.__archclass__[arch](self.usage_params[u])
			#print('***\n',u,'--->',self.engine[u]['args'])

	def add_engines(self,xml_str,arch='VLC'):
		if not isinstance(xml_str,dict):
			#usage_params = StreamingMedia.__usage_param(StringIO(xml_str))
			import io 
			usage_params = StreamingMedia.usage_param(io.StringIO(xml_str))
		else:
			usage_params = xml_str

		for u in usage_params:
			if u in self.engine:
				print("Warning: Engine of the ID`s task is already exist,old one will be delete")
				#return False
				self.del_engines(u)
				#self.engine[u].shutdown()
				#del self.engine[u]
				#del self.usage_params[u]

		self.usage_params.update(usage_params)
		for u in usage_params:
			self.engine[u] = StreamingMedia.__archclass__[arch](self.usage_params[u])
			#print('+++\n',u,'-->',self.engine[u]['args'])

		return True

	def del_engines(self,id):
		self.engine[id].shutdown()
		del self.engine[id]
		del self.usage_params[id]
		pass
		
	def engines(self):
		return self.engine

	def start(self,id):
		self.engine[id].start();
		pass
	
	def stop(self, id):
		self.engine[id].stop();
		pass

	def restart(self, id):
		self.stop(id)
		self.start(id)
		pass

	def set_event(self, id, state,func,args):
		self.engine[id].set_event(state,func,args)

	def check_status(self,id):
		return self.engine[id].checking_status()

	def get_track_synchronization_time(self,id):
		pass

	def get_timing(self,id):
		return self.usage_params[id][0]['timing']

	def get_encoding_state(self,id):
		return self.usage_params[id][0]['encoding_state']

	def get_caching(self,id):
		return self.usage_params[id][0]['caching']

	@staticmethod
	def usage_param(xml_filename):
			"""
				usage_streaming = {id: ({input description}, [(),(),(), ...])}
				usage_streaming =      	 		      /\
				usage_streaming = 					
				usage_streaming = {id: ({url dict},[({video},{audio},[{output},{output}],),({video},{audio},{output}),...]) }
			"""
			def extract_param(out_param, keysmap):
				for engine_param in keysmap:
					if  keysmap[engine_param] == '':
						continue
					#print keysmap[engine_param]
					value_str = keysmap[engine_param]
					#print value_str
					try:
						out_param[engine_param] = route.find(value_str).text
					except :
						print('Warnning: param will be set NULL', engine_param)
						#out_param[engine_param]=''

					if engine_param == 'venc':
						out_param['venc'] ='x264'
					if engine_param == 'vcodec':
						out_param['vcodec'] ='h264'
					if engine_param == 'acodec':
						out_param['aenc'] ='ffmpeg'
						out_param['acodec'] ='mp4a'
				#checking args

			usage_streaming = {}
			keysmap_app_to_vlc_video={'venc':'library','vcodec':'vcodec','width':'width','height':'height', \
									'fps':'frame-num','vb':'bitrate', \
									'vbv-maxrate':'vbv-maxbitrate',\
									'vbv-bufsize':'vbv-capacity','threads':'threads',\
									'scenecut':'dynamic-Iframe', 'keyint':'gop', \
									'profile':'profile','bframes':'bframes','qpmax':'video-quality',\
									'deinterlace':'deinterlace-fields','interlace':'interlace','bitratemode':'bitratemode',\
									'logo-file':'logofile','x':'logoxpos','y':'logoypos','logoalpha':'logoalpha'}
			keysmap_app_to_vlc_audio={'aenc':'', 'acodec':'acodec', 'channels':'channels',\
									'samplerate':'frequency','ab':''}
			#keysmap_app_to_vlc_output={'mux':'', 'access_out':'outudp', 'access_iface':'outaddr'}
			
			import xml.etree.ElementTree as ET
			try:
				tree = ET.parse(xml_filename)
				root = tree.getroot()
				channel = root.findall('channels')[0];
				channels = [ele for ele in channel.findall('channel')]
				for chan in channels:
					chanid = chan.find('id').text
					url = chan.find('uri').text
					if url[:3] != 'udp' and url[:3] != 'rts' and url[:3] != 'rtm':
						continue;
					iface = chan.find('iface').text
					program = chan.find('program-number').text
					#deinterlace= chan.find('deinterlace-fields').text
					audio_track= chan.find('audioinfo').text.split(',')[0]
					try:
						audio_track = int(audio_track)
						audio_track = str(audio_track)
					except:
						audio_track = '-1'
					#print audio_track
					try:
						logpath= chan.find('logpath').text
					except :
						logpath = None
					try:
						debuglevel = chan.find('debuglevel').text
					except:
						debuglevel = None
					try:
						caching = chan.find('caching').text
					except:
						caching = None
					try:
						timing = chan.find('timing').text
					except:
						timing = None
					try:
						encoding_state = chan.find('encoding-state').text
					except:
						encoding_state = None
					usage_streaming[chanid] = ({'chanid':chanid,'url':url,'iface':iface,'prgid':program,'logpath':logpath,'debuglevel':debuglevel,'caching':caching,'timing':timing,'encoding_state':encoding_state,'atrack':audio_track}, [])
					#usage = UsageTree(url + ' ' + iface)
					encoder = chan.findall('encoders')[0]
					encoders = [enc for enc in encoder.findall('encoder')]
					for route in encoders:
						video_param = {}
						audio_param = {}
						subtt_param = {}
						output_param = []
						extract_param(video_param,keysmap_app_to_vlc_video)

						#print '++++' ,video_param 
						extract_param(audio_param,keysmap_app_to_vlc_audio)
						temp_x = [o for o in route.findall('outudp')]
						temp_y = [o for o in route.findall('outaddr')]
						i=0
						for x in temp_x:
							output_param.append({'access_out':x.text,'access_iface':temp_y[i].text})
							i+=1
						#extract_param(output_param,keysmap_app_to_vlc_output)
						usage_streaming[chanid][1].append((video_param, audio_param,subtt_param, output_param))
			except Exception as e: 
				print("xml error !!!",e.message)
				exit()
			return usage_streaming
	#usage_param = staticmethod(usage_param )
	#usage_param = classmethod(usage_param)
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
	os.environ['VLC_PLUGIN_PATH']='/home/smsd/modules'
	os.environ['LD_LIBRARY_PATH']='/home/smsd/modules:/home/hebin/.temp/cerbero/build/dist/linux_x86_64/lib:/home/hebin/.temp/cerbero/deps/installed/lib:/home/hebin/.temp/Gcc/installed/lib64'
	#print os.environ['LD_LIBRARY_PATH']
	streamer = StreamingMedia()
	config_file = '/etc/itvencoder/itvencoder.xml'
	#print("=====>")
	args_lists=wsgi.parser_config(config_file)
	#print("=====>",args_lists)
	streamer.add_engines(args_lists[0]['master'],'GST')
	print([i for i in streamer.engine])
	id = argv[1]
	streamer.start(id)
	#streamer.set_event(id,2001,event_Func,None)
	import time
	while 1:
		#print(streamer.check_status(id))
		time.sleep(1.0)	
