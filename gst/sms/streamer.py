import copy
import gstengine as Gst
import vlcengine as Vlc

class StreamingMedia(): 
	""" 
		Focus on statistic data, status, control about a stream object
	"""
	__archclass__ = {'GST':Gst.EngineGST,'VLC':Vlc.EngineVLC}
	def __init__(self,usage_config=None,arch ='GST'):
		try:
			import usage
			if usage.rootdir == '' or usage.rootdir is None:
				self.rootdir = '.'
			else:
				self.rootdir = usage.rootdir
		except Excetion as e:
			print("Program don't be installed successful")
			exit()
		if not usage_config:
			self.usage_params = {}
		else:
			self.usage_params = StreamingMedia.usage_param(usage_config)
		#for u in self.usage_params:
		#	print self.usage_params[u]
		self.engine = {}
		
		for u in self.usage_params:
			self.usage_params[u][0]['rootdir']=self.rootdir
			self.engine[u] = StreamingMedia.__archclass__[arch](self.usage_params[u])
			#print('***\n',u,'--->',self.engine[u]['args'])

	def add_engines(self,xml_str,arch='GST'):
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
			self.usage_params[u][0]['rootdir']=self.rootdir
			if self.usage_params[u][1][0][0]['vcodec']=='h264':
				arch = 'VLC'
			print("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx1",self.rootdir,arch)
			self.engine[u] = StreamingMedia.__archclass__[arch](self.usage_params[u])
			print("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx2")
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
	def get_encoders_arch(usage):
		encoders = usage[0]['encoders']
		arch='GST'
		if encoders == 'smsv':	
			arch = 'VLC'
		elif encoders == 'smsg':
			arch = 'GST'
		return arch
	
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
	import binding
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
	id = argv[1]
	#os.environ['LD_LIBRARY_PATH']='/home/smsd/modules:/home/hebin/.temp/cerbero/build/dist/linux_x86_64/lib:/home/hebin/.temp/cerbero/deps/installed/lib:/home/hebin/.temp/Gcc/installed/lib64' # no effect
	#print os.environ['LD_LIBRARY_PATH']
	streamer = StreamingMedia()
	config_file = '/etc/itvencoder/itvencoder.xml'
	#print("=====>")
	args_lists=wsgi.parser_config(config_file)
	encoders = args_lists[0]['master'][id][0]['encoders']
	print("=====>",encoders)
	arch = streamer.get_encoders_arch(args_lists[0]['master'][id])
		
	streamer.add_engines(args_lists[0]['master'],arch)
	print([i for i in streamer.engine])
	streamer.start(id)
	#streamer.set_event(id,2001,event_Func,None)
	import time
	while 1:
		#print(streamer.check_status(id))
		time.sleep(1.0)	
