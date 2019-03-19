import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
import copy
import engineer#.Engineer as Engineer
import chain

class EngineGST(engineer.Engineer):
	#			caps-lib-cat
	__ele_list={'udp--src':				{'name':'udpsrc','propertys':		{'uri':''}},
				'file--src':			{'name':'filesrc','propertys':		{'location':''}},
				'typefind--typefind':	{'name':'typefind','propertys':		{}},
				'tee--dup':				{'name':'tee','propertys':			{}},
				'--demux':				{'name':'unkown','propertys':		{}},
				'ts--demux':			{'name':'tsdemux','propertys':		{}},
				'mp4--demux':			{'name':'qtdemux','propertys':		{}},
				'demux_decode--decode':	{'name':'decodebin','propertys':	{}},
				'mpeg--vpreparse':		{'name':'mpegvideoparse','propertys':{}},
				'h264--vpreparse':		{'name':'h264parse','propertys':	{}},
				'h265--vpreparse':		{'name':'h265parse','propertys':	{}},
				'mpeg--apreparse':		{'name':'mpegaudioparse','propertys':{}},
				'aac--apreparse': 		{'name':'aacparse','propertys':		{}},
				'ac3--apreparse': 		{'name':'ac3parse','propertys':		{}},
				'dvd--spreparse': 		{'name':'dvdsubparse','propertys':	{}},
				'dvb--spreparse': 		{'name':'subparse','propertys':		{}},
				'--vconvert': 			{'name':'autovideoconvert','propertys':{}},
				'--aconvert': 			{'name':'audioconvert','propertys':{}},
				'--sconvert': 			{'name':'','propertys':				{}},
				'h264-x264-vencode':	{'name':'x264enc','propertys':		{}},
				'h264-openh264-vencode':		{'name':'openh264enc','propertys':{}},
				'h265-x265-vencode':	{'name':'x265enc','propertys':{}},
				'aac-fdkaac-aencode': 	{'name':'fdkaacenc','propertys':{}},
				'spu-unkown-sencode': 	{'name':'unkown','propertys':{}},
				'ts--wrap': 			{'name':'mpegtsmux','propertys':{'alignment':7}},
				'mp4--wrap': 			{'name':'qtmux','propertys':{}},
				'udp--access_out': 		{'name':'udpsink','propertys':{'host':'','port':0,'multicast-iface':''}},
				'fil--access_out': 	{'name':'filesink','propertys':{'location':''}},
				}
	def __init__(self,usage):
		engineer.Engineer.__init__(self,'GST', usage)
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
		self.__build_pipeline__()

	def __config_element__(self,caps,lib,cat,e):
		chain = self.chainner
		propertys = e.element_propertys
		
		if cat == 'src':
			if caps == 'udp':
				if lib == '':
					e.element_propertys['uri']=self['url']
			elif caps == 'fil':
				if lib == '':
					e.element_propertys['location']=self['url'][8:]
		elif cat == 'aencode':
			l= e.name.split('_')#aencode_aac_fdkaac_3 
			ll = len(l)
			stream_num = int(l[ll-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][1]
			if caps == 'aac':
				if lib== 'fdkaac':
					e.element_propertys['bitrate']=int(args['ab'])
			#####
			queue = Gst.ElementFactory.make('queue')
			queue.set_property('max-size-buffers',0) 
			queue.set_property('max-size-time',0) 
			queue.set_property('max-size-bytes',0)
			el=chain.Element_link('queue'+e.element_name)
			el.set_caps('queue')
			el.set_mod_name('queue')
			el.element_name= el.name
			el.element_obj = queue
			e.last.append(el)
			#####
			p ='audio/x-raw'
			plb = len(p)
			for a in args:
				if a == 'channels':
					p += (',channels='+args[a])
				elif a == 'samplerate':
					p += (',rate='+args[a])
			pla = len(p)
			print(p)
			if plb != pla:
				capsfilter = Gst.ElementFactory.make('capsfilter','afilter_%s'%stream_num)
				cap_property = Gst.Caps.from_string(p)
				capsfilter.set_property('caps',cap_property)
				el=chain.Element_link('afilter_%s'%stream_num)
				el.set_caps('capsfilter')
				el.set_mod_name = 'capsfilter'
				el.element_name= el.name
				el.element_obj = capsfilter
				e.last.append(el)
		elif cat == 'vencode':
			l= e.name.split('_')#vencode_h264_x264_3 
			ll = len(l)
			stream_num = int(l[ll-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][0]
			if caps == 'h264':
				if lib== 'x264':
					e.element_propertys['bitrate']=int(args['vb'])
					e.element_propertys['bframes']=int(args['bframes'])
					e.element_propertys['vbv-buf-capacity']=int(1000/float(args['fps'])) ## cbr or vbr or abr
					e.element_propertys['byte-stream']= True ## cbr or vbr or abr
					e.element_propertys['key-int-max']= int(args['keyint'])## cbr or vbr or abr
			#####
			queue = Gst.ElementFactory.make('queue')
			queue.set_property('max-size-buffers',0) 
			queue.set_property('max-size-time',0) 
			queue.set_property('max-size-bytes',0)
			e.last.append(queue)
			#####
			p ='video/x-raw'
			plb = len(p)
			for a in args:
				if a == 'width':
					p += (',width='+args[a])
				elif a == 'height':
					p += (',height='+args[a])
				elif a == 'fps':
					p += (',framerate=%s/1'%args[a])
			pla = len(p)
			print(p)
			if plb != pla:
				videoscale = Gst.ElementFactory.make('videoscale','vscale_%s'%stream_num)
				capsfilter = Gst.ElementFactory.make('capsfilter','vfilter_%s'%stream_num)
				cap_property = Gst.Caps.from_string(p)
				capsfilter.set_property('caps',cap_property)
				e.last.append(videoscale)
				e.last.append(capsfilter)
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
						multicast_iface = args['access_iface']
						e.element_propertys['multicast-iface']=multicast_iface
					e.element_propertys['host']=host
					e.element_propertys['port']=int(port)
		elif cat == 'demux_decode':
			queue = Gst.ElementFactory.make('queue')
			queue.set_property('max-size-buffers',0) 
			queue.set_property('max-size-time',0) 
			queue.set_property('max-size-bytes',0)
			e.last.append(queue)
		elif cat == 'dup':
			pass
		else:
			print('elemnt ',e.name,' no property config')
					
		
	def __build_element__(self,caps_lib_cat,e):
		caps = caps_lib_cat.split('-')[0]
		lib = caps_lib_cat.split('-')[1]
		cat = caps_lib_cat.split('-')[2]
		e.caps_name = caps 
		e.mod_name = EngineGST.__ele_list[caps_lib_cat]['name']
		e.element_name = e.name
		if e.name != 'demux':
			e.element_obj = Gst.ElementFactory.make(e.mod_name,e.name)
			e.element_propertys=EngineGST.__ele_list[caps_lib_cat]['propertys']
			self.__config_element__(caps,lib,cat,e)
			for pp in e.element_propertys:
				e.element_obj.set_property(pp,e.element_propertys[pp])
	
	def __build_pipeline__(self):
		el_pool = self.chainner.el_pool
		ell = self.chainner.element_link_lists
		pipeline = self['pipeline']
            
		for e in el_pool:
			if e == 'access_in':
				self.__build_element__(self.access+'--src',el_pool[e])
			elif e[:3] == 'tee':
				self.__build_element__('tee--dup',el_pool[e])
			elif e == 'typefind' :
				self.is_dynamic = True 
				self.__build_element__(e + '--typefind',el_pool[e])
				el_pool[e].element_obj.connect("have-type",EngineGST.__findedtype, self)
			elif e == 'demux' :
				self.is_dynamic = True 
				self.__build_element__('--demux',el_pool[e])
			elif e == 'demux_decode' :
				self.__build_element__(e +'--decode',el_pool[e])
			elif e == 'vconvert' :
				self.__build_element__('--vconvert',el_pool[e])
			elif e == 'aconvert' :
				self.__build_element__('--aconvert',el_pool[e])
			elif e == 'sconvert' :
				self.__build_element__('--sconvert',el_pool[e])
			elif e[:4] == 'aenc' :
				self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-aencode',el_pool[e])
			elif e[:4] == 'venc' :
				self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-vencode',el_pool[e])
			elif e[:4] == 'senc' :
				self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-sencode',el_pool[e])
			elif e == 'apreparse' :
				self.is_dynamic = True 
				#self.__build_element__('v_'+'unkown'+'-preparse',el_pool[e])
			elif e == 'vpreparse' :
				self.is_dynamic = True 
			elif e == 'spreparse' :
				self.is_dynamic = True 
			elif e[:4] == 'wrap' :
				self.__build_element__(e.split('-')[1]+'--wrap',el_pool[e])
			elif e[:10] == 'access_out':
				self.__build_element__(e.split('-')[1]+'--access_out',el_pool[e])
			else:
				print("Something wrong!!!")

			if not el_pool[e].element_obj:
				print('Obj of ',e,' is null!!!' )
			else:
				ret = pipeline.add(el_pool[e].element_obj)
				if ret:
					print('add element',e)

		if self.is_dynamic:
			print('This is dynamic links')

		for el in ell:
			print('link:>')
			head=None;cur=None;next=None;#tail=None;
			#last != next means have no tee
			linked=None;linking=None;
			for e in el:
				cur = el[e]
				if not head:
					head = cur
					next = head	
					continue
				elif e[:3] == 'tee':
					pass
				elif e == 'typefind ' or e == 'demux':
					head = cur = None
					continue
				else :
					head = cur

				linked = head
				linking= cur
				for i in linking.last:	
				 	ret=linked.element_obj.link(i)
				 	if not ret:
				 		print('linked ', linked.name,'==>',i.name ,' failure')
				 	linked=i
					
				if linked.element_obj is not None:
					ret=linked.element_obj.link(linking.element_obj)
					if not ret:
						print('linked ', linked.name,'==>',linking.name,' failure')
				
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

if __name__ == '__main__':
	gstengineer = EngineGST(chain.usage)
