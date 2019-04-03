import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
import copy
import engineer#.Engineer as Engineer
import chain
import os,sys
import _thread
count =0
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
				'vconvert--vconvert': 			{'name':'autovideoconvert','propertys':{}},
				'aconvert--aconvert': 			{'name':'audioconvert','propertys':{}},
				'sconvert--sconvert': 			{'name':'','propertys':				{}},
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
		os.environ["GST_DEBUG_DUMP_DOT_DIR"] = "/tmp"
		os.putenv('GST_DEBUG_DUMP_DIR_DIR', '/tmp')
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
					
		
	def __build_element__(self,caps_lib_cat,e):
		caps = caps_lib_cat.split('-')[0]
		lib = caps_lib_cat.split('-')[1]
		cat = caps_lib_cat.split('-')[2]
		e.caps_name = caps 
		try:
			e.mod_name = EngineGST.__ele_list[caps_lib_cat]['name']
		except Exception as ex:
			print('xxxx',ex)
			return None
		e.element_name = e.name
		if caps != '' and e.mod_name != '':
			e.ele_obj = Gst.ElementFactory.make(e.mod_name,e.name)
			e.element_propertys=EngineGST.__ele_list[caps_lib_cat]['propertys']
			self.__config_element__(caps,lib,cat,e)
			for pp in e.element_propertys:
				e.ele_obj.set_property(pp,e.element_propertys[pp])
		return e

	def insert_queue(self,e,direct='last',name=''):
		el=chain.Element_link('queue-'+e.element_name)
		if name == '':
			queue = Gst.ElementFactory.make('queue','queue-'+e.element_name)
		else:
			global count;
			print("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$",count)
			count += 1;
			queue = Gst.ElementFactory.make('queue')
			
		queue.set_property('max-size-buffers',0) 
		queue.set_property('max-size-time',0) 
		queue.set_property('max-size-bytes',0)
		el.set_caps('queue')
		el.set_mod_name('queue')
		el.element_name= el.name
		el.ele_obj = queue
		if direct =='last':
			e.last.append(el)
		elif direct == 'next':
			e.next.append(el)
		else:
			return el
	
	def __config_element__(self,caps,lib,cat,e):
		propertys = e.element_propertys
		
		if cat == 'src':
			if caps == 'udp':
				if lib == '':
					e.element_propertys['uri']=self['url']
			elif caps == 'fil':
				if lib == '':
					e.element_propertys['location']=self['url'][8:]
		elif cat == 'typefind':
			e.ele_obj.connect("have-type",EngineGST.findedtype, self)
			self.insert_queue(e)
		elif cat == 'demux' :
			if lib == '':
				e.ele_obj.connect("pad-added",EngineGST.pad_added, self)
		elif cat == 'aencode':
			l= e.name.split('_')#aencode_aac_fdkaac_3 
			ll = len(l)
			stream_num = int(l[ll-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][1]
			if caps == 'aac':
				if lib== 'fdkaac':
					e.element_propertys['bitrate']=int(args['ab'])
			#####
			self.insert_queue(e)
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
			#if False:#plb != pla:
			if plb != pla:
				el=chain.Element_link('aresample_%s'%stream_num)
				el.ele_obj= Gst.ElementFactory.make('audioresample','aresample_%s'%stream_num)
				el.element_name= el.name
				el.caps_property = Gst.Caps.from_string(p)
				print('---><<',el.caps_property.to_string())
				el.set_caps('audioresample')
				el.set_mod_name = 'audioresample'
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
			self.insert_queue(e)
			el=chain.Element_link('videoscale_%s'%stream_num)
			el.ele_obj= Gst.ElementFactory.make('videoscale','videoscale_%s'%stream_num)
			el.set_caps('videoscale')
			el.set_mod_name('videoscale')
			el.element_name= el.name
			e.last.append(el)
			#####
			p ='video/x-raw,format=I420'
			plb = len(p)
			for a in args:
				if a == 'width':
					p += (',width='+args[a])
				elif a == 'height':
					p += (',height='+args[a])
				elif a == 'fps':
					p += (',framerate=%s/1'%args[a])
			pla = len(p)
			#if False:
			if plb != pla:
				el=chain.Element_link('videocapsfilter_%s'%stream_num)
				el.ele_obj= Gst.ElementFactory.make('capsfilter','videocapsfilter_%s'%stream_num)
				vcaps = Gst.Caps.from_string(p)
				el.ele_obj.set_property("caps", vcaps)
				#help(el.ele_obj)
				el.set_caps('vcapsfilter')
				el.set_mod_name('capsfilter')
				el.element_name= el.name
				e.last.append(el)
		elif cat == 'access_out':
			l= e.name.split('-')#access_out-udp-0_2-ts
			ll = len(l)
			stream_num = int(l[ll-2].split('_')[1])
			out_num = int(l[ll-2].split('_')[0])
			args= self.chainner.pattern_lists[stream_num]['args_list'][3][out_num]
			if caps == 'fil':
				if lib == '':
					e.element_propertys['location']=args['access_out'][7:]
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
					print(host,port)
		elif cat == 'decode':
			self.insert_queue(e)
			e.ele_obj.connect("pad-added",EngineGST.pad_added, self)
		elif cat == 'vpreparse' or cat == 'apreparse' or cat == 'spreparse':
			self.insert_queue(e)
		elif cat == 'vconvert' or cat == 'aconvert' or cat == 'sconvert':
			self.insert_queue(e)
		elif cat == 'dup':
			pass
		else:
			print('elemnt ',e.name,' no property config')

	def __build_pipeline__(self):
		el_pool = self.chainner.el_pool
		#ell = self.chainner.element_link_lists
		pipeline = self['pipeline']
            
		for e in el_pool:
			if e == 'access_in':
				self.__build_element__(self.access+'--src',el_pool[e])
				ret = pipeline.add(el_pool[e].ele_obj)
				if ret:
					print('add element',e)
			elif e[:3] == 'tee':
				self.__build_element__('tee--dup',el_pool[e])
			elif e == 'typefind' :
				self.is_dynamic = True 
				self.__build_element__(e + '--typefind',el_pool[e])
			elif e == 'demux' :
				self.is_dynamic = True 
				self.__build_element__('--demux',el_pool[e])
			elif e == 'demux_decode' :
				self.__build_element__(e +'--decode',el_pool[e])
			elif e == 'vconvert' :
				self.__build_element__('vconvert--vconvert',el_pool[e])
			elif e == 'aconvert' :
				self.__build_element__('aconvert--aconvert',el_pool[e])
			elif e == 'sconvert' :
				self.__build_element__('sconvert--sconvert',el_pool[e])
			elif e[:4] == 'aenc' :
				self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-aencode',el_pool[e])
			elif e[:4] == 'venc' :
				self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-vencode',el_pool[e])
			elif e[:4] == 'senc' :
				self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-sencode',el_pool[e])
			elif e == 'apreparse' :
				self.is_dynamic = True 
				self.__build_element__('--apreparse',el_pool[e])
			elif e == 'vpreparse' :
				self.is_dynamic = True 
				self.__build_element__('--vpreparse',el_pool[e])
			elif e == 'spreparse' :
				self.is_dynamic = True 
				self.__build_element__('--spreparse',el_pool[e])
			elif e[:4] == 'wrap' :
				self.__build_element__(e.split('-')[1]+'--wrap',el_pool[e])
			elif e[:10] == 'access_out':
				self.__build_element__(e.split('-')[1]+'--access_out',el_pool[e])
			else:
				print("Something wrong!!!")

			if not el_pool[e].ele_obj:
				print('Obj of ',e,' is null!!!' )
			#else:
			#	print("ADDing 1^^^^",el_pool[e].element_name)
			#	ret = pipeline.add(el_pool[e].ele_obj)
			#	if ret:
			#		print('add element',e)
			el = el_pool[e]
		
		if self.is_dynamic:
			print('This is dynamic links')
		
		for key in el_pool:
			#print("start link >>:", key)
			e = el_pool[key]
			linked=None;linking=None;
			if e.name  == 'access_in':
				continue
			elif e.name == 'typefind' or e.name == 'tee_in' or e.name == 'demux_decode':
				linked=el_pool[e.up_name]
				linking=e
				self.linked_link(linked,linking)
		
	def linked_link(self,linked,linking):
		if 'access_out' in linking.name:
			if not linking.linked_done:
				linking.linked_done = True
			else:
				return
		if linked.name[:4] == 'tee_' and linked.name != 'tee_in':
			print('BBBBBBBBB',linked.name)
			e_queue = self.insert_queue(linked,None,None)
			print('linkqueue>> ',linked.name,' ',e_queue.name)
			print("ADDing 1^^^^",e_queue.element_name)
			self['pipeline'].add(e_queue.ele_obj)
			ret=linked.ele_obj.link(e_queue.ele_obj)
			if not ret:
				print('linkin ', linked.name,'==>',e_queue.name ,' failure')
			e_queue.ele_obj.set_state(Gst.State.PLAYING)
			linked = e_queue
	
		for i in linking.last:
			print('linkin>> ',linked.name,' ',i.name)
			print("ADDing 2^^^^",i.element_name)
			self['pipeline'].add(i.ele_obj)
			if not i.caps_property :
				ret=linked.ele_obj.link(i.ele_obj)
			else:
				ret=linked.ele_obj.link_filtered(i.ele_obj,i.caps_property)
			if not ret:
				print('linkin ', linked.name,'==>',i.name ,' failure')
			i.ele_obj.set_state(Gst.State.PLAYING)
			linked=i
					
		if linked.ele_obj is not None:
			print('linking>>',linked.name,linking.name)
			print("ADDing 3^^^^",linking.element_name)
			self['pipeline'].add(linking.ele_obj)
			ret=linked.ele_obj.link(linking.ele_obj)
			if not ret:
				print('linked ', linked.name,'==>',linking.name,' failure')
			linking.ele_obj.set_state(Gst.State.PLAYING)
		else:
			print("OBJ is None")
			
	@staticmethod
	def pad_added(src, new_pad,obj):
		pipeline  = obj['pipeline']
		ep = obj.chainner.el_pool
		new_pad_caps = Gst.Pad.get_current_caps(new_pad)
		new_pad_struct = new_pad_caps.get_structure(0)
		new_pad_type = new_pad_struct.get_name()
		new_pad_name = new_pad.get_name()
		cas_lib_cat = ''
		e_name = src.get_name()
		e=None

		print("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&   Received new pad '%s' type '%s' from '%s'" % (new_pad_name,new_pad_type,src.get_name()))
		if 'vpreparse' not in ep and 'video' in new_pad_type:
			if 'vconvert' not in ep:
				return
		if 'apreparse' not in ep and 'audio' in new_pad_type:
			if 'aconvert' not in ep:
				return
		if new_pad_type == 'video/x-h265':
			caps_lib_cat = 'h265'+'--vpreparse'
			e = ep['vpreparse']
		elif new_pad_type == 'video/x-h264':
			caps_lib_cat = 'h264'+'--vpreparse'
			e = ep['vpreparse']
		elif new_pad_type == 'video/mpeg':
			caps_lib_cat = 'mpeg'+'--vpreparse'
			e = ep['vpreparse']
		elif new_pad_type == 'audio/mpeg':
			caps_lib_cat = 'mpeg'+'--apreparse'
			e = ep['apreparse']
		elif new_pad_type == 'audio/aac':
			caps_lib_cat = 'aac'+'--apreparse'
			e = ep['apreparse']
		elif new_pad_type == 'audio/x-ac3':
			caps_lib_cat = 'ac3'+'--apreparse'
			e = ep['apreparse']
		elif new_pad_type == 'audio/x-raw':
			if 'aconvert' in ep:
				e = ep['aconvert']
		elif new_pad_type == 'video/x-raw':
			e = ep['vconvert']
		else:
			print(new_pad_type,' is not support')
			exit()
		if e_name == 'demux':
			ret = obj.__build_element__(caps_lib_cat,e)
			if ret is None:
				print("parse failed",)
				exit()
		obj.linked_link(ep[e_name],e)
		
		#if e_name == 'demux':
		#	return
		print("After start link >>;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;:" )
		for key in ep:
			print(' ++++++++ ',key,' ++++++++ ')
			e = ep[key]
			if e_name == 'demux' and 'convert' in key:
				return
			linked=None;linking=None;
			if e.mod_name == '' or e.caps_name == '':
				print('This is dynamic link')
				continue
			elif e.name  == 'access_in' or e.name  == 'typefind' or e.name == 'demux' or e.name == 'demux_decode' or e.name == 'tee_in':
				continue
			elif e.up_name == '':
				print('last element is null')
				continue
			elif key[0:4] == 'wrap':
				list = e.up_name.split('-')
				print(list)
				for i in range(0,3):
					if list[i] != '' and ep[list[i]].mod_name != '':
						if new_pad_type[0] == list[i][4] or new_pad_type[0] == list[i][0]:
							obj.linked_link(ep[list[i]],e)
			elif ep[e.up_name].mod_name == '' or ep[e.up_name].caps_name == '':
				print('can`t be link ', e.up_name,e.name)
				continue
			elif 'preparse' in e.name:
				continue
			else:
				linked=ep[e.up_name]
				linking=e
				obj.linked_link(linked,linking)
		#for i in ep:
		#	e = ep[i]
		#	print(e.name,'\t\t\t\t==>',e.mod_name,e.element_name,'--up-name:',e.up_name)
		Gst.debug_bin_to_dot_file_with_ts(pipeline, Gst.DebugGraphDetails.ALL, 'pipeline')
		print('Done')

	@staticmethod
	def findedtype(typefinder,probability,caps,obj):
		pipeline  = obj['pipeline']
		ep = obj.chainner.el_pool
		e=ep['demux']
		caps_lib_cat =''
		size = caps.get_size()
		print('caps size :',size)
		#Gst.debug_bin_to_dot_file_with_ts(pipeline, Gst.DebugGraphDetails.ALL, 'pipeline')
		for i in range(size):
			structure =  caps.get_structure(i)
			name = structure.get_name()
			print('find new type ',name)
			if name == 'video/mpegts':
				#obj= Gst.ElementFactory.make("tsdemux","demuxer")
				caps_lib_cat = 'ts'+'--demux'
			elif name == 'video/quicktime':
				#obj = Gst.element_factory_make("qtdemux","demuxer")
				caps_lib_cat = 'mp4'+'--demux'
			else:
				print('No support ',name)
				exit()
		ret = obj.__build_element__(caps_lib_cat,e)
		if ret is None:
			print("typefind failed")
			exit()
		#print("ADDing 4^^^^",e.element_name)
		#ret = pipeline.add(e.ele_obj)
		#if ret is None:
		#	print('add demux error')
		obj.linked_link(ep['typefind'],e)
		#e.ele_obj.set_state(Gst.State.PLAYING)
		#pipeline.set_state(Gst.State.PAUSED)
		#pipeline.set_state(Gst.State.PLAYING)
		
		
	def __bus_call(bus, message, data):
		t = message.type
		if t == Gst.MessageType.EOS:
			sys.stdout.write("End-of-stream\n")
			Gst.debug_bin_to_dot_file_with_ts(data[1], Gst.DebugGraphDetails.ALL, 'caps')
			print("End of ")
			data[0].quit()
			#pipeline.set_state(Gst.State.READY)
			#pipeline.set_state(Gst.State.PLAYING)
		elif t == Gst.MessageType.ERROR:
			err, debug = message.parse_error()
			sys.stderr.write("Error: %s: %s\n" % (err, debug))
			print("Error ")
			data[0].quit()
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
		_thread.start_new_thread(EngineGST.__start,(self,None))
		return True

	def __start(self,notuse):
		self['bus'].add_signal_watch()
		self['bus'].connect("message", EngineGST.__bus_call, (self['mainloop'],self['pipeline']))
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
	#help(Gst.debug_bin_to_dot_file_with_ts)
	gstengineer = EngineGST(chain.usage)
	gstengineer.start()
	import time
	time.sleep(10000)

