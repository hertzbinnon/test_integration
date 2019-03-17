import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
import copy
import engineer
import chain

class EngineGST(Engineer):
	#			caps-lib-cat
	__ele_list={'udp--src':				{'name':'udpsrc','propertys':		{'uri':''}},
				'file--src':			{'name':'filesrc','propertys':		{'location':''}},
				'typefind--typefind':	{'name':'typefind','propertys':		{}
				'tee--dup':				{'name':'tee','propertys':			{}
				'ts--demux':			{'name':'tsdemux','propertys':		{}
				'mp4--demux':			{'name':'qtdemux','propertys':		{}
				'demux_decode--decode':	{'name':'decodebin','propertys':	{}
				'mpeg--vpreparse':		{'name':'mpegvideoparse','propertys':{}
				'h264--vpreparse':		{'name':'h264parse','propertys':	{}
				'h265--vpreparse':		{'name':'h265parse','propertys':	{}
				'mpeg--apreparse':		{'name':'mpegaudioparse','propertys':{}
				'aac--apreparse': 		{'name':'aacparse','propertys':		{}
				'ac3--apreparse': 		{'name':'ac3parse','propertys':		{}
				'dvd--spreparse': 		{'name':'dvdsubparse','propertys':	{}
				'dvb--spreparse': 		{'name':'subparse','propertys':		{}
				'--vconvert': 			{'name':'autovideoconvert','propertys':{}
				'--aconvert': 			{'name':'audioconvert','propertys':{}
				'--sconvert': 			{'name':'','propertys':				{}
				'h264-x264-vencode':	{'name':'x264enc','propertys':		{}
				'h264-openh264-vencode':		{'name':'openh264enc','propertys':{}
				'h265-x265-vencode':	{'name':'x265enc','propertys':{}
				'aac-fdkaac-aencode': 	{'name':'fdkaacenc','propertys':{}
				'spu-unkown-sencode': 	{'name':'unkown','propertys':{}
				'ts--wrap': 			{'name':'mpegtsmux','propertys':{'alignment':7}
				'mp4--wrap': 			{'name':'qtmux','propertys':{}
				'udp--access_out': 		{'name':'udpsink','propertys':{'host':'','port':0,'multicast-iface':''}
				'fil--access_out': 	{'name':'filesink','propertys':{'location':''}
				}
	def __init__(self,usage):
		Engineer.__init__(self,'GST', usage)
		self['arch']={}
		self['arch']['gst'] = Gst
		self['arch']['gobj'] = GObject 
		GObject.threads_init()
		Gst.init(None)
		if usage[0]['url'][:3] == 'udp' and usage[0]['url'][6] != '@':
			usage[0]['url'] = 'udp://@' + usage[0]['url'][6:]
		self['pipeline']   = self['arch']['gst'].Pipeline(name='itv-channel-' + usage[0]['chanid'])
		self['mainloop'] = self['arch']['gobj'].MainLoop()
		self['bus'] = self['pipeline'].get_bus()
		self['get_state'] = None
		self['url'] = usage[0]['url']
		self.access = usage[0]['url'][:3] 
		self['args']  = ''
		self.is_dynamic = False
		self.chainner = chain.Chain(usage[1])
		self.__build_pipeline()

	def __config_element__(self,caps,lib,cat,e):
		chain = self.chainner
		propertys = e.element_propertys
		
		if cat == 'src':
			if caps == 'udp':
				if lib == '':
					e.element_propertys['uri']=self['url']
			elif caps == 'fil'
				if lib == '':
					e.element_propertys['location']=self['url'][8:]
		elif cat == 'aencode':
			l= e.name.split('_')#aencode_aac_fdkaac_3 
			ll = len(l)
			stream_num = int(l[len(1)-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][1]
			if caps == 'aac':
				if lib== 'fdkaac':
					e.element_propertys['bitrate']=int(args['ab'])
			#####
			p ='audio/x-raw,%s,%s'
			plb = len(p)
			for a in args:
				if a == 'channels':
					p = p % ('channels='+args[a])
				elif a == 'samplerate':
					p = p % ('rate='+args[a])
			pla = len(p)
			if plb != pla:
				capsfilter = Gst.ElementFactory.make('capsfilter','afilter_%s'%stream_num)
				cap_property = Gst.Caps.from_string(p)
				capsfilter.set_property('caps',cap_property)
				e.last = capsfilter
		elif cat == 'vencode':
			l= e.name.split('_')#vencode_h264_x264_3 
			ll = len(l)
			stream_num = int(l[len(1)-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][1]
			if caps == 'h264':
				if lib== 'x264':
					e.element_propertys['bitrate']=int(args['bitrate'])
					e.element_propertys['bframes']=int(args['bframes'])
					e.element_propertys['vbv-buf-capacity']=(1000/float(args['fps'])) ## cbr or vbr or abr

			#####
		elif cat == 'access_out':
			l= e.name.split('-')#access_out-udp-0_2-ts
			ll = len(l)
			stream_num = int(l[ll-2].split('_')[1])
			out_num = int(l[ll-2].split('_')[0])
			args= self.chainner.pattern_lists[stream_num]['args_list'][3][out_num]
			if caps == 'fil':
				if lib == '':
					e.element_propertys['location']=args['access_out'][8:]
			elif caps == 'udp':
				if lib == '':
					l = args['access_out'].split('/')[2]
					host = l.split(':')[0]
					port = l.split(':')[1]
					if 'access_iface' in args and args['access_iface'] != '':
						multicast-iface = args['access_iface']
						e.element_propertys['multicast-iface']=multicast-iface
					e.element_propertys['host']=host
					e.element_propertys['port']=int(port)
					
		
	def __build_element__(self,caps_lib_cat,e):
		caps = caps_cat.split('-')[0]
		lib = caps_cat.split('-')[1]
		cat = caps_cat.split('-')[2]
		e.caps_name = caps 
		e.mod_name = __ele_list[cat-caps]['name']
		e.element_name = e.name
		e.element_obj = Gst.ElementFactory.make(e.mod_name,e.name)
		e.element_propertys=__ele_list[cat-caps]['propertys']
		self.__config_property__(caps,lib,cat,e)
		for pp in e.element_propertys:
			e.element_obj.set_property(pp,e.element_propertys[pp])
	
	def __build_pipeline__(self):
		el_pool = chain.el_pool
		ell = chain.element_link_lists
		pipeline = self['pipeline']
            
		for e in el_pool:
			if e == 'access_in':
				__build_element(self.access+'-src',el_pool[e])
			elif e[:3] == 'tee':
				__build_element('tee--dup',el_pool[e])
			elif e == 'typefind' :
				self.is_dynamic = True 
				__build_element(e + '--typefind',el_pool[e])
				el_pool[e].connect("have-type", EngineGST.__findedtype, self)
			elif e == 'demux' :
				self.is_dynamic = True 
			elif e == 'demux_decode' :
				__build_element(e +'--decode',el_pool[e])
			elif e[:4] == 'aenc' :
				__build_element(e.split('_')[1]+'-'+e.split('_')[2]+'-aencode',el_pool[e])
			elif e[:4] == 'venc' :
				__build_element(e.split('_')[1]+'-'+e.split('_')[2]+'-vencode',el_pool[e])
			elif e[:4] == 'senc' :
				__build_element(e.split('_')[1]+'-'+e.split('_')[2]+'-sencode',el_pool[e])
			elif e == 'apreparse' :
				self.is_dynamic = True 
				#__build_element('v_'+'unkown'+'-preparse',el_pool[e])
			elif e == 'vpreparse' :
				self.is_dynamic = True 
			elif e == 'spreparse' :
				self.is_dynamic = True 
			elif e[:4] == 'wrap' :
				__build_element(e.split('-')[1]+'--wrap',el_pool[e])
			elif e[:10] == 'access_out':
				__build_element(e.split('-')[1]+'--access_out',el_pool[e])

			pipeline.add(e.element_obj)
				




			self.ele_access_in = Gst.ElementFactory.make("udpsrc","access")
			print(self['url'])
			self.ele_access_in.set_property('uri',self['url'])
			print(usage[0]['iface'])
			self.ele_access_in.set_property('multicast-iface',usage[0]['iface'])
			self.ele_typefind = Gst.ElementFactory.make("typefind","typefinder")
			self.ele_typefind.connect("have-type", EngineGST.__findedtype, self)
		"""
		elif access_in == 'file':
			access = Gst.element_factory_make("filesrc","access")
			access.set_property('location','/media/hertz/b901d1b9-1b63-46ca-a706-86f7401fee63/hebin/4K 体验视频.mp4')
			typefind = Gst.element_factory_make("typefind","typefinder")
			#demuxer= Gst.element_factory_make("tsdemux","demuxer")
		elif access_in == 'rtsp':
			access = Gst.element_factory_make("rtspsrc","access")
			access.set_property('location','rtsp://192.168.61.26/')
			typefind = Gst.element_factory_make("typefind","typefinder")
		elif access_in == 'rtmp':
			access = Gst.element_factory_make("rtmpsrc","access")
		elif access_in == 'http':
			access = Gst.element_factory_make("httpsrc","access")
			access.set_property('location','http://192.168.61.26/')
			typefind = Gst.element_factory_make("typefind","typefinder")
			typefind.connect("have-type", findedtype, pipeline)
		"""
		if self.ele_typefind is not None:
			self['pipeline'].add(self.ele_access_in)
			self['pipeline'].add(self.ele_typefind)
			self.ele_access_in.link(self.ele_typefind)

	def __build_link(gstobject,track_str,track_name):
		pipeline  = gstobject['pipeline']

	def __pad_added(src, new_pad, gstobject):
		pipeline  = gstobject['pipeline']

	def __findedtype(typefinder,probability,caps,gstobject):
		pipeline  = gstobject['pipeline']

	def __bus_call(bus, message, loop):
		t = message.type
		if t == Gst.MessageType.EOS:
			sys.stdout.write("End-of-stream\n")
			loop.quit()
			print("End of ")
			#pipeline.set_state(Gst.State.READY)
			#pipeline.set_state(Gst.State.PLAYING)
		elif t == Gst.MessageType.ERROR:
			err, debug = message.parse_error()
			sys.stderr.write("Error: %s: %s\n" % (err, debug))
			print("Error ")
			loop.quit()
		elif t == Gst.MessageType.STATE_CHANGED:
			#sys.stdout.write("stream\n")
			pass
		return True

	def shutdown(self):
		self['pipeline'].set_state(Gst.State.NULL)

	def stop(self):
		self['pipeline'].set_state(Gst.State.PAUSED)
		return True

	def start(self):
		import _thread
		_thread.start_new_thread(EngineGST.__start,(self,None))
		return True

	@staticmethod
	def __start(data,notuse):
		self = data
		self['bus'].add_signal_watch()
		self['bus'].connect("message", EngineGST.__bus_call, self['mainloop'])
		self['pipeline'].set_state(Gst.State.PLAYING)
		#GObject.timeout_add(5*1000, switch_file, loop)
		try:
			print("Sta of ")
			self['mainloop'].run()
			print("End of ")
		except Exception as e:
			print("streamer except exit by",e.message)
		self['pipeline'].set_state(Gst.State.NULL)

	def set_event(self,state,func,args):
		pass
	def checking_status(self):
		return 0

