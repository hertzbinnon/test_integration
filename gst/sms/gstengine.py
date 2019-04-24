import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
import copy
import engineer#.Engineer as Engineer
import chain
import os,sys
import _thread

class EngineGST(engineer.Engineer):
	#			caps-lib-cat
	__ele_list={'udp--src':				{'name':'udpsrc','propertys':		{'uri':'','buffer-size':'120000000'}},
				'file--src':			{'name':'filesrc','propertys':		{'location':''}},
				'typefinder--typefinder':	{'name':'typefind','propertys':		{}},
				'tee--dup':				{'name':'tee','propertys':			{}},
				'--demux':				{'name':'unkown','propertys':		{}},
				'ts--demux':			{'name':'tsdemux','propertys':		{}},
				'mp4--demux':			{'name':'qtdemux','propertys':		{}},
				'demux_decode--decode':	{'name':'decodebin','propertys':	{'max-size-bypes':'100000000'}},
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
				'mp4a-ffmpeg-aencode': 	{'name':'avenc_aac','propertys':{}},
				'spu-unkown-sencode': 	{'name':'unkown','propertys':{}},
				'ts--wrap': 			{'name':'mpegtsmux','propertys':{'alignment':'7'}},
				'mp4--wrap': 			{'name':'qtmux','propertys':{}},
				'udp--access_out': 		{'name':'udpsink','propertys':{'host':'','port':'0','multicast-iface':''}},
				'fil--access_out': 	{'name':'filesink','propertys':{'location':''}},
				}
	def __init__(self,usage):
		engineer.Engineer.__init__(self,'GST', usage)
		self['arch']={}
		self['arch']['gst'] = Gst
		self['arch']['gobj'] = GObject 
		self['id']=usage[0]['chanid']
		self['rootdir']=usage[0]['rootdir']
		os.environ['GST_PLUGIN_PATH']=self['rootdir']+'/modules'

		logpath_str=''
		log_root = ''
		log_format =  '/itv-channel-' + self['id']

		if 'logpath' in usage and usage['logpath']:
			log_root = usage['logpath']+ log_format
			logpath_str = log_root  + '/smsd.log'
		else:
			log_root = self['rootdir']+'/log'+log_format
			logpath_str = log_root+ '/smsd.log'
		
		print("Yxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
		try:
			os.makedirs(log_root)
			print('cccccctv................................',log_root)
		except Exception as e:
			print('Log direcory already exist', e)

		os.putenv('GST_DEBUG_FILE',logpath_str)# effect
		#os.putenv('GI_TYPELIB_PATH',self['rootdir']+'modules/girepository-1.0') 
		#os.putenv('GST_PLUGIN_SCANNER',self['rootdir']+'bin/gst-plugin-scanner')

		log_level = '0'
		if 'debuglevel' in usage[0] and usage[0]['debuglevel'] != None:
			log_level = usage[0]['debuglevel']
		else:
			usage[0]['debuglevel'] = log_level

		if log_level == '0':
			os.environ['GST_DEBUG']='1'# effect
		elif log_level == '1':
			os.environ['GST_DEBUG']='2'# effect
		elif log_level == '2':
			os.environ['GST_DEBUG']='3'# effect
			os.putenv('GST_DEBUG_DUMP_DIR', '/tmp')
			os.environ["GST_DEBUG_DUMP_DOT_DIR"] = "/tmp"

		GObject.threads_init()
		#help(Gst.init)
		Gst.init(None)

		if usage[0]['url'][:3] == 'udp' and usage[0]['url'][6] != '@':
			usage[0]['url'] = 'udp://@' + usage[0]['url'][6:]
		self['mainloop'] = self['arch']['gobj'].MainLoop()
		self['get_state'] = None
		self['url'] = usage[0]['url']
		self.access = usage[0]['url'][:3] 
		self['args']  = ''
		self.is_dynamic = False
		self.chainner = chain.Chain(usage[1])
		self.str_launch=''
		self.__build_pipeline__()
		#self['pipeline'] = None;
		#self['bus'] = None;
					
		
	def __build_element__(self,caps_lib_cat,e):
		caps = caps_lib_cat.split('-')[0]
		lib = caps_lib_cat.split('-')[1]
		cat = caps_lib_cat.split('-')[2]
		e.caps_name = caps 
		try:
			e.mod_name = EngineGST.__ele_list[caps_lib_cat]['name']
		except Exception as ex:
			print('No libs found',ex)
			return exit()
		e.element_name = e.name
		e.element_propertys=EngineGST.__ele_list[caps_lib_cat]['propertys']
		self.__config_element__(caps,lib,cat,e)
		if e.to_string != '':
			e.to_string = e.to_string + ' ! ' + e.mod_name +' name=' + e.element_name+' '
		else:
			e.to_string = e.mod_name +' name=' + e.element_name+' '
		for pp in e.element_propertys:
			e.to_string += (pp+'='+e.element_propertys[pp]+' ')
		print('string::',e.to_string)
		return e

	def __config_element__(self,caps,lib,cat,e):
		propertys = e.element_propertys
		
		if cat == 'src':
			if caps == 'udp':
				if lib == '':
					e.element_propertys['uri']=self['url']
			elif caps == 'fil':
				if lib == '':
					e.element_propertys['location']=self['url'][8:]
		elif cat == 'typefinder':
			#e.ele_obj.connect("have-type",EngineGST.findedtype, self)
			pass
		elif cat == 'demux' :
			#if lib == '':
			#	e.ele_obj.connect("pad-added",EngineGST.pad_added, self)
			pass
		elif cat == 'aencode':
			l= e.name.split('_')#aencode_aac_fdkaac_3 
			ll = len(l)
			stream_num = int(l[ll-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][1]
			if caps == 'aac':
				if lib== 'fdkaac':
					e.element_propertys['bitrate']=args['ab']
			elif caps == 'mp4a':
				#if lib== 'avenc_aac':
				#	e.element_propertys['bitrate']=args['ab']
				pass
			p ='audio/x-raw'
			plb = len(p)
			for a in args:
				if a == 'channels':
					p += (',channels='+args[a])
				elif a == 'samplerate':
					p += (',rate='+args[a])
			pla = len(p)
			if pla == plb:
				return None
			print('OOOOOOOOOOOOOx',e.to_string)
			e.to_string = 'audioresample name=asampler-'+str(stream_num)+ ' ! '+p+' ' 
			print('OOOOOOOOOOOOOO',e.to_string)
		elif cat == 'vencode':
			l= e.name.split('_')#vencode_h264_x264_3 
			ll = len(l)
			stream_num = int(l[ll-1])
			args= self.chainner.pattern_lists[stream_num]['args_list'][0]
			self.config_vencoder(args)
			if caps == 'h264':
				if lib== 'x264':
					e.element_propertys['bitrate']=args['vb']
					e.element_propertys['bframes']=args['bframes']
					e.element_propertys['vbv-buf-capacity']=str(int(1000/float(args['fps']))) ## cbr or vbr or abr
					e.element_propertys['byte-stream']= 'True' ## cbr or vbr or abr
					e.element_propertys['key-int-max']= args['keyint']## cbr or vbr or abr
			elif caps == 'h265':
				if lib == 'x265':
					e.element_propertys['log-level']= '2'
					e.element_propertys['speed-preset']= '0'
					e.element_propertys['tune']= '0'
					e.element_propertys['option-string']= \
						'bitrate='+args['vb'] +\
						':bframes='+args['bframes'] +\
						':vbv-bufsize='+args['vbv-bufsize'] +\
						':vbv-maxrate='+args['vbv-maxrate'] +\
						':qpmax='+args['qpmax'] +\
						':scenecut='+args['scenecut'] +\
						':keyint='+args['keyint'] +':min-keyint=25' +':ctu=16'\
						':info=true' 
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
			if pla == plb:
				return
			print('NNNNNNNNNNNNNx',e.to_string)
			e.to_string = 'videoscale name=vscaler-'+str(stream_num)+ ' ! '+p+' ' 
			if 'deinterlace' in args and args['deinterlace']=='1':
				e.to_string += '! deinterlace name=deinterlace-'+str(stream_num)+ ' mode=1 method=5' + ' ! queue '
			print('NNNNNNNNNNNNNN',e.to_string)
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
					e.element_propertys['port']=port
					print(host,port)
		elif cat == 'decode':
			pass
		elif cat == 'vpreparse' or cat == 'apreparse' or cat == 'spreparse':
			pass
		elif cat == 'vconvert' or cat == 'aconvert' or cat == 'sconvert':
			pass
		elif cat == 'dup':
			pass
		else:
			print('elemnt ',e.name,' no property config')

	def __build_pipeline__(self):
		el_pool = self.chainner.el_pool
            
		for e in el_pool:
			up_name = el_pool[e].up_name
			if e == 'access_in':
				eo=self.__build_element__(self.access+'--src',el_pool[e])
				self.str_launch=eo.to_string
			elif e[:3] == 'tee':
				eo=self.__build_element__('tee--dup',el_pool[e])
				if 'tee_in' in e:
					self.str_launch += (' ! ' + eo.to_string)
				elif 'tee_wrap' in e:
					self.str_launch += (eo.up_name + '. ! '+eo.mod_name+' name='+eo.name+' ')
				elif 'tee_' in e:
					if 'trans' not in e:
						self.str_launch += eo.up_name + '. ! '+' '+eo.to_string+' '
					else:
						self.str_launch += ' '+eo.to_string+' '
			elif e == 'typefinder' :
				eo=self.__build_element__(e + '--typefinder',el_pool[e])
				if up_name == 'tee_in':
					self.str_launch += (' tee_in. ! queue name=tee-queue-typefinder max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! ' + eo.to_string)
				else:
					self.str_launch += (' ! ' + eo.to_string)
			elif e == 'demux' :
				self.is_dynamic = True 
				eo=self.__build_element__('--demux',el_pool[e])
			elif e == 'demux_decode' :
				eo=self.__build_element__(e +'--decode',el_pool[e])
				if up_name == 'tee_in':
					self.str_launch += (' tee_in. ! queue name=tee-queue-demux_decode max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! ' + eo.to_string)
				else:
					self.str_launch += (' ! queue ! ' + eo.to_string)
			elif e == 'vconvert' :
				eo=self.__build_element__('vconvert--vconvert',el_pool[e])
				self.str_launch += up_name+'. ! queue ! ' + eo.to_string  
			elif e == 'aconvert' :
				eo=self.__build_element__('aconvert--aconvert',el_pool[e])
				self.str_launch += up_name+'. ! queue ! ' + eo.to_string
			elif e == 'sconvert' :
				eo=self.__build_element__('sconvert--sconvert',el_pool[e])
				self.str_launch += up_name+'. ! ' + eo.to_string
			elif e[:4] == 'aenc' :
				eo=self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-aencode',el_pool[e])
				self.str_launch += up_name+'. ! ' + eo.to_string 
			elif e[:4] == 'venc' :
				eo=self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-vencode',el_pool[e])
				self.str_launch += up_name+'. ! ' + eo.to_string 
			elif e[:4] == 'senc' :
				eo=self.__build_element__(e.split('_')[1]+'-'+e.split('_')[2]+'-sencode',el_pool[e])
				self.str_launch += up_name+'. ! ' + eo.to_string 
			elif e == 'apreparse' :
				self.__build_element__('--apreparse',el_pool[e])
			elif e == 'vpreparse' :
				self.__build_element__('--vpreparse',el_pool[e])
			elif e == 'spreparse' :
				self.__build_element__('--spreparse',el_pool[e])
			elif e[:4] == 'wrap' :
				eo=self.__build_element__(e.split('-')[1]+'--wrap',el_pool[e])
				list = eo.up_name.split('-');print(list)
				llen = 0;muxer=None;
				idlist = eo.name.split('-');print(idlist)
				id = idlist[len(idlist)-1]
				for i in list:
					if i != '':
						llen +=1
						if llen == 1:
							muxer=' '+eo.to_string+' '
						up = el_pool[i]
						if up.to_string != '':
							self.str_launch += up.name+'. ! queue name=queue-' +up.name +'-'+id+' ! ' + eo.name+'. '
				self.str_launch+= muxer
				#print('====>',self.str_launch)
			elif e[:10] == 'access_out':
				eo=self.__build_element__(e.split('-')[1]+'--access_out',el_pool[e])
				if 'tee_wrap' in up_name  :
					self.str_launch += ' ' + up_name+'. ! queue name=queue-'+eo.name+' ! ' + eo.to_string +' '
				else:
					self.str_launch += ' ' + up_name+'. ! ' + eo.to_string +' '
			else:
				print("Something wrong!!!")
		
		print('====>',self.str_launch)
		self['pipeline'] = Gst.parse_launch(self.str_launch)
		self['bus'] = self['pipeline'].get_bus()

		if not self.is_dynamic:
			return 

		print('This is dynamic links')
		pipeline = self['pipeline']
		typefinder = pipeline.get_by_name('typefinder')
		typefinder.connect('have-type',EngineGST.findedtype,self)

	@staticmethod
	def linking(src,sink):
		ret = src.link(sink)
		if not ret:
			print('linking',src,' ==> ',sink,'error')

	@staticmethod
	def findedtype(typefinder,probability,caps,obj):
		pipeline  = obj['pipeline']
		ep = obj.chainner.el_pool
		e=ep['demux']
		caps_lib_cat =''
		size = caps.get_size()
		print('caps size :',size)
		for i in range(size):
			structure =  caps.get_structure(i)
			name = structure.get_name()
			print('find new type ',name)
			if name == 'video/mpegts':
				demuxer = Gst.ElementFactory.make("tsdemux","demuxer")
				caps_lib_cat = 'ts'+'--demux'
				demuxer.connect("pad-added",EngineGST.pad_added, obj)
				pipeline.add(demuxer)
				EngineGST.linking(typefinder,demuxer)
				demuxer.set_state(Gst.State.PLAYING)
			elif name == 'video/quicktime':
				demuxer = Gst.element_factory_make("qtdemux","demuxer")
				caps_lib_cat = 'mp4'+'--demux'
			else:
				print('No support ',name)
				exit()
		ret = obj.__build_element__(caps_lib_cat,e)
		if ret is None:
			print("typefinder failed")
		obj.str_launch+= (e.up_name+'. ! '+ e.mod_name + ' name=' +e.name)
		#Gst.debug_bin_to_dot_file_with_ts(pipeline, Gst.DebugGraphDetails.ALL, 'gstpipeline')
		
		
	@staticmethod
	def pad_added(src, new_pad,obj):
		pipeline  = obj['pipeline']
		ep = obj.chainner.el_pool
		new_pad_caps = Gst.Pad.get_current_caps(new_pad)
		new_pad_struct = new_pad_caps.get_structure(0)
		new_pad_type = new_pad_struct.get_name()
		new_pad_name = new_pad.get_name()
		e_name = src.get_name()

		cas_lib_cat = ''
		e=None

		print("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&   Received new pad '%s' type '%s' from '%s'" % (new_pad_name,new_pad_type,src.get_name()))

		if new_pad_type[:5] == 'video':
			if 'vpreparse' not in ep:
				return
		elif new_pad_type[:5] == 'audio':
			if 'apreparse' not in ep:
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
		else:
			print(new_pad_type,' is not support')
			exit()
		ret = obj.__build_element__(caps_lib_cat,e)
		if ret is None:
			print("parse failed",)
			exit()
		preparse = Gst.ElementFactory.make(e.mod_name,e.name)
		pipeline.add(preparse)
		queue = Gst.ElementFactory.make('queue',e.up_name+'-queue-'+e.name)
		pipeline.add(queue)
		EngineGST.linking(src,preparse)
		EngineGST.linking(preparse,queue)

		next = None
		eo=None
		for i in ep:
			eo = ep[i]
			print('^...^',eo.name,eo.mod_name,'==>',eo.up_name)
			if  eo.name == 'tee_atrans':
				if new_pad_type[:5] == 'audio':
					next = pipeline.get_by_name(eo.name)
					break
			if  eo.name == 'tee_vtrans':
				if new_pad_type[:5] == 'video':
					next = pipeline.get_by_name(eo.name)
					break
			if 'apreparse' in eo.up_name and eo.name[0:5] == 'wrap-':
				if new_pad_type[:5] == 'audio':
					next = pipeline.get_by_name(eo.name)
					break
			if 'vpreparse' in eo.up_name and eo.name[0:5] == 'wrap-':
				if new_pad_type[:5] == 'video':
					next = pipeline.get_by_name(eo.name)
					break

		if not next:
			print('Pipeline linking failed ?????')
			exit()
		print(next.name)
		EngineGST.linking(queue,next)
		preparse.set_state(Gst.State.PLAYING)
		queue.set_state(Gst.State.PLAYING)

		obj.str_launch+= (' '+e.up_name+'. ! '+ e.mod_name+' name='+e.name + ' ! queue name='  + e.name+'-queue-'+eo.name+' ! '+eo.name+'.')
		print("===<",obj.str_launch)
		#Gst.debug_bin_to_dot_file_with_ts(pipeline, Gst.DebugGraphDetails.ALL, 'pipeline')
	
	def __bus_call(bus, message, data):
		t = message.type
		if t == Gst.MessageType.EOS:
			sys.stdout.write("End-of-stream\n")
			#Gst.debug_bin_to_dot_file_with_ts(data[1], Gst.DebugGraphDetails.ALL, 'caps')
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
		else:
			print('Message:',t)
		return True

	def shutdown(self):
		self['pipeline'].set_state(Gst.State.NULL)

	def stop(self):
		#self['pipeline'].set_state(Gst.State.PAUSED)
		self['pipeline'].set_state(Gst.State.READY)
		return True

	def start(self):
		_thread.start_new_thread(EngineGST.__start,(self,None))
		return True

	def __start(self,notuse):
		self['bus'].add_signal_watch()
		self['bus'].connect("message", EngineGST.__bus_call, (self['mainloop'],self['pipeline']))
		self['pipeline'].set_state(Gst.State.PLAYING)
		Gst.debug_bin_to_dot_file_with_ts(self['pipeline'], Gst.DebugGraphDetails.ALL, self['id'])
		#GObject.timeout_add(5*1000, switch_file, loop)
		try:
			self['mainloop'].run()
		except Exception as e:
			print("streamer except exit by",e.message)
		self['pipeline'].set_state(Gst.State.NULL)

	def set_event(self,state,func,args):
		pass
	def checking_status(self):
		pipeline=self['pipeline']#.state_get_name()
		state = pipeline.get_state(1000)[1]
		class stateENU():
			def __init__(self,value,state):
				self.value = value
				self.state = state
		so = stateENU(2,state)
		return so
	def config_vencoder(self,args):
		print('------....',args)
		if args['bitratemode'] == '1':
			args['bframes'] = '1'
		if args['qpmax'] == '4':
			args['qpmax']= '36'
		elif args['qpmax']== '3': 
			args['qpmax']= '41'
		elif args['qpmax']== '2': 
			args['qpmax'] = '46'
		elif args['qpmax']== '1': 
			args['qpmax'] = '51'
		elif args['qpmax']== 'cbr-ex': 
			args['qpmax'] = '37'
			args['vb']=str( int(int(args['vb']) / 0.90)  )
			args['vbv-maxrate']=args['vb']
			args['vbv-bufsize']=str(int(int(args['vb'])/int(args['fps'])))
			args['bframes'] = '0'
		else:
			args['qpmax']='32'
		from multiprocessing import cpu_count
		threads = cpu_count()
		print('------....',args)
		if args['threads'] == '0':
			args['threads']=str(threads/2+threads/4+threads/8)


if __name__ == '__main__':
	#help(Gst.debug_bin_to_dot_file_with_ts)
	#help(gi.repository.Gst.Element)
	#LIBS_PRIVATE='/home/smsd/modules:/home/hebin/.temp/cerbero/build/dist/linux_x86_64/lib:/home/hebin/.temp/cerbero/deps/installed/lib:/home/hebin/.temp/Gcc/installed/lib64'
	#os.environ['LD_LIBRARY_PATH']=LIBS_PRIVATE
	#os.pwtenv('LD_LIBRARY_PATH', LIBS_PRIVATE)
	gstengineer = EngineGST(chain.usage)
	gstengineer.start()
	import time
	time.sleep(10000)

