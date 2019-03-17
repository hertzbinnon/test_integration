#!/usr/bin/python3
import os,sys
import copy
"""
	gst-launch-1.0 -e -v udpsrc uri=udp://@192.168.61.26:12346 ! tee name=teer_in ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! decodebin name=decoder !  queue !  autovideoconvert ! tee name=tee_vcodec ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! x264enc !  queue ! mpegtsmux name=muxer alignment=7 ! udpsink host=192.168.61.22 port=22345 decoder. ! queue !  audioconvert ! tee name=tee_acodec ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0   ! fdkaacenc ! queue ! muxer. teer_in. !  queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  !  typefind ! tsdemux name=demuxer !  queue ! mpegvideoparse ! queue ! mpegtsmux name=muxer1 alignment=7 ! udpsink host=192.168.61.22 port=22346 demuxer. ! queue ! mpegaudioparse ! queue ! muxer1. tee_vcodec. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! x264enc !  queue ! mpegtsmux name=muxer2 alignment=7 ! udpsink host=192.168.61.22 port=22347 tee_acodec. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0   ! fdkaacenc ! queue ! muxer2.
"""
def string_sub(First,Second):
	for i in First :
		if i in Second :
			First = First.replace(i,"")
	print( First )
# NOTE 
class Element_link():
	def __init__(self,name=None):
		if name is None or not isinstance(name,str) or name not in stream_pattern:
			#print("must be in ",stream_pattern) 
			pass
		self.name=name # eg. access_in
		self.caps_name=''
		self.mod_name=''
		self.element_name=''
		self.element_obj=None
		self.element_propertys={}
		self.next =None#
		self.last =None#
		self.queue=None#{'name':'queue','property':'queue max-size-buffers=0 max-size-time=0 max-size-bytes=0'}
	def set_caps(self,caps):
		self.caps_name=caps
	def set_mod_name(self,mod):
		self.mod_name=mod
	def set_obj(self,obj):
		self.element_obj=obj
	def set_propertys(self,propertys):
		self.element_propertys=copy.deepcopy(propertys)
	def set_last(self,last):
		self.last=last
	def set_next(self,next):
		self.next=next

stream_pattern=[
	'access_in',
	'tee_in',
	'demux_decode' ,#decodebin
	'typefind' ,
	'demux',
	'apreparse',
	'vpreparse',
	'spreparse',
	#'adecode',
	#'vdecode',
	#'sdecode',
	'aconvert',
	'vconvert',
	'sconvert',
	#'acapsfilter'
	#'vcapsfilter'
	#'scapsfilter'
	'tee_acon',
	'tee_vcon',
	'tee_scon',
	'aencode_%s_%d',    
	'vencode_%s_%d',
	'sencode_%s_%d',
	'tee_atrans',#
	'tee_vtrans',#
	'tee_strans',#
	'tee_aen_%d',#
	'tee_ven_%d',#
	'tee_sen_%d',#
	'tee_wrap_%d',
	'wrap_%d_%d',
	'wrap_%d_%d',
	'wrap_%d_%d',
	# ...
	'access_out-%d_%d',
	'access_out-%d_%d',
	'access_out-%d_%d',
	# ...
	]
class Chain():
	def __init__(self,lists=None,pattens=None):
		"""
			lists=[streaming,streaming,streaming,streaming,streaming,...]
			streaming=(video_codec_conf,audio_codec_conf,subtitle_codec_conf,output_wraper_conf)
			video_codec_config={'codec':'h264',...}
			audio_codec_config={'codec':'aac'}
		subtitle_codec_config={'codec':'pgs'}
		output_wraper_config=[access_out,access_out]
		access_out={'dst':'http://192.168.0.1','access_iface':'eth0','container':'ts'}
		"""
		self.streams  = 0
		self.vcodecs  = 0
		self.acodecs  = 0
		self.scodecs  = 0
		self.vtrans= 0
		self.atrans= 0
		self.strans= 0
		self.access_out_total    = 0
		self.tee_in = False
		#self.tee_acon= False
		#self.tee_vcon= False
		#self.tee_scon= False
		#self.tee_aes= False
		#self.tee_ves= False
		#self.tee_ses= False
		#self.tee_aen= False
		#self.tee_ven= False
		#self.tee_sen= False
		self.pattern_lists =[]#[{'id':id,'pattern':[],'args_list':'','tee_track_num':%d,'wrap_num':%d,'tee_wrap_num':%d},{}]
		self.element_link_lists =[]#[{'access_in':elobj},{}]
		self.el_pool={}
		if lists is not None and isinstance(lists,list):
			self.__config_pasing__(lists)

	def __track_parse__(self,track,track_type):
		if len(track) > 2:#transcode
			if track_type == 'v':
				self.vcodecs +=1
			elif track_type == 'a':
				self.acodecs +=1
			elif track_type == 's':
				self.scodecs +=1
		else:
			if len(track) == 0:#transform
				if track_type == 'v':
					self.vtrans +=1
				elif track_type == 'a':
					self.atrans +=1
				elif track_type == 's':
					self.strans +=1
			elif len(track) == 1: # no track
				pass
			else:
				print('config for'+ track_type +'is vaild')
	
	def __config_pasing__(self,lists):
		self.streams=len(lists)
		stream_id = 0
		for args in lists:
			stream_pattern={}
			stream_pattern['id'] = stream_id
			stream_pattern['pattern'] = ['access_in']
			stream_pattern['args_list'] = args
			self.__track_parse__(args[0],'v')
			self.__track_parse__(args[1],'a')
			self.__track_parse__(args[2],'s')
			if len(args[3]) < 1:
				print('no access out find')
			self.access_out_total += len(args[3])
			self.pattern_lists.append(stream_pattern)
			del(stream_pattern)
			stream_id += 1

		if self.streams < self.access_out_total:
			print('Wrapper may be multiplexing')

		pl = self.pattern_lists
		if  self.vcodecs != 0 and self.vtrans != 0 or \
			self.acodecs != 0 and self.atrans != 0 or \
			self.scodecs != 0 and self.strans != 0 :
			self.tee_in = True
			if self.tee_in :
				for sp in pl:
					sp['pattern'].append('tee_in')
			
		#for sp in self.pattern_lists:
		#	print(sp)
		# 000 111 001 100 010 011 101 110 _01
		#if self.vcodecs != 0 or self.acodecs != 0 or self.scodecs != 0:
		for sp in pl:
			vlen = len(sp['args_list'][0])
			alen = len(sp['args_list'][1])
			slen = len(sp['args_list'][2])
			if vlen > 2 and alen > 2 and slen > 2 : # 111
				sp['pattern'].append('demux_decode')
			elif vlen == 0 and alen == 0 and slen == 0 : # 000
				sp['pattern'].append('typefind')
				sp['pattern'].append('demux')
			else:
				if vlen >= 1 and vlen < 2 or alen >= 1 and alen < 2 or slen >= 1 and slen < 2:# no track
					if (vlen > 2 or alen > 2 or slen > 2) and (vlen == 0 or alen == 0 or slen == 0):# _01
						sp['pattern'].append('demux_decode')
						sp['pattern'].append('typefind')
						sp['pattern'].append('demux')
					else:
						sp['pattern'].append('demux_decode')
				else:
					sp['pattern'].append('typefind')
					sp['pattern'].append('demux')

		for sp in pl:
			args = sp['args_list']
			vlen = len(args[0])
			alen = len(args[1])
			slen = len(args[2])
			mlen = len(args[3])
			if vlen > 1 :
				sp['pattern'].append('vconvert')
				if self.vcodecs > 1:
					sp['pattern'].append('tee_vcon')
				sp['pattern'].append('vencode_%s_%s_%d' % (sp['args_list'][0]['vcodec'],sp['args_list'][0]['venc'],sp['id']))
			elif vlen == 0 :
				sp['pattern'].append('vpreparse')
				if self.vtrans > 1 or mlen>1:
					sp['pattern'].append('tee_vtrans')
			else:
				pass

			if alen > 1 :
				sp['pattern'].append('aconvert')
				if self.acodecs > 1:
					sp['pattern'].append('tee_acon')
				sp['pattern'].append('aencode_%s_%s_%d'% (sp['args_list'][1]['acodec'],sp['args_list'][1]['aenc'],sp['id']))
			elif alen == 0 :
				sp['pattern'].append('apreparse')
				if self.vtrans > 1 or mlen>1:
					sp['pattern'].append('tee_atrans')
			else:
				pass

			if slen > 1 :
				sp['pattern'].append('sconvert')
				if self.scodecs > 1:
					sp['pattern'].append('tee_scon')
				sp['pattern'].append('sencode_%s_%s_%d'% (sp['args_list'][2]['scodec'],sp['args_list'][2]['senc'],sp['id']))
			elif slen == 0 :
				sp['pattern'].append('spreparse')
				if self.strans > 1 or mlen>1:
					sp['pattern'].append('tee_strans')
			else:
				pass

			if mlen > 1:
				if vlen > 1:
					sp['pattern'].append('tee_ven_%d' % sp['id'])
				if alen > 1:
					sp['pattern'].append('tee_aen_%d' % sp['id'])
				if slen > 1:
					sp['pattern'].append('tee_sen_%d' % sp['id'])

			sp['tee_track_num']=mlen
			sp['wrap_num']=mlen
			sp['tee_wrap_num']=0
			for i in range(mlen):
				sp['pattern'].append('wrap_%s_%d_%d'%(args[3][i]['mux'],i,sp['id']))
				sp['pattern'].append('access_out-%s-%d_%d'%(args[3][i]['access_out'][:3],i,sp['id']))
			#print('==',sp['pattern'])
			#print('**',sp['args_list'],'\n')
			
		# merge muxing and outputing
		#mux_caps_vorg_aorg_sorg__id1_id2_...
		for sp in pl:
			pattern=sp['pattern']
			args   = sp['args_list']
			plen=pattern
			stream_id=0;index=0;aorg='';vorg='';sorg='';
			for p in pattern:
				if 'apreparse' in p:
					aorg = p
				elif 'tee_atrans' in p:
					aorg = p
				elif 'aencode' in p:
					aorg = p
				elif  'tee_aen' in p:
					aorg = p
				
				if 'vpreparse' in p:
					vorg = p
				elif 'tee_vtrans' in p:
					vorg = p
				elif 'vencode' in p:
					vorg = p
				elif 'tee_ven' in p:
					vorg = p

				if 'spreparse' in p:
					sorg = p
				elif 'tee_strans' in p:
					sorg = p
				elif 'sencode' in p:
					sorg = p
				elif 'tee_sen' in p:
					sorg = p
			#print(aorg,vorg,sorg) 
			#sp['muxing_temp']=aorg+'-'+vorg+'-'+sorg
		#for sp in pl:
			#pattern=sp['pattern']
			for p in pattern:
				if 'wrap_' in p :
					index=pattern.index(p)
					#print(index,p)
					l = p.split('_')
					caps = l[1]
					suffix=''
					for i in range(2,len(l)):
						if suffix=='':
							suffix='%s_'%l[i]
						else:
							suffix +='%s'%l[i]
					pattern[index]='wrap-'+caps+'-'+aorg+'-'+vorg+'-'+sorg+'-'+suffix
			#print('+++++++',sp['pattern'])
			#print('**',sp['args_list'],'\n')
			keys={}
			wrap_index_list=[]
			token=[]
			for p in pattern:
				if 'wrap-' in p:
					wrap_index_list.append(pattern.index(p))
			for i in wrap_index_list:
				t = pattern[i].split('-')
				#print('>>',t)
				string=''
				for s in range(len(t)-1):
					if s != len(t)-2:
						string+=(t[s]+'-')
					else:
						string+=(t[s])
				token.append(string)
			#print('>>',token)
			for t in token:
				keys[t]=[]
			#print('<<',keys)
			sp['wrap_num']= len(keys)
			sp['tee_track_num'] = len(keys)
			aorg = ''
			vorg = ''
			sorg = ''
			#print('-------',sp['pattern'])
			if len(keys) == 1 and len(sp['args_list'][3]) != 1: # merge tee_track
				for key in keys.items():
					p = key[0].split('-')
					aorg = p[2]
					vorg = p[3]
					sorg = p[4]
					print(aorg,vorg,sorg)
					tmp = copy.deepcopy(pattern)
					for p in tmp:
						if aorg == p or vorg == p or sorg == p:
							if aorg != 'tee_atrans' and vorg != 'tee_vtrans' and sorg != 'tee_strans' :
								pattern.remove(p)
			#print('\n',sp['args_list'])			
			#print('=======',sp['pattern'],'\n')			

			#merge output
			#sp['tee_wrap_num'] =  
			#if len(token) != len(keys) :#and len(sp['args_list'][3]) != 1:
			if True:
				out_index_list=[]
				wrap_index_list=[]
				tee_wrap = ''
				for p in pattern:
					if 'access_out' in p:
						out_index_list.append(pattern.index(p))
					if 'wrap-' in p:
						wrap_index_list.append(pattern.index(p))
				for key in keys:
					for i in wrap_index_list:
						if key in pattern[i]:
							keys[key].append(i)
					if len(keys[key]) > 1:
						tee_wrap='tee_wrap-%s' % key.split('-')[1]
						first =1
						for i in keys[key]:
							if first == 1:
								pattern[i] = key+'-'+str(sp['id'])
								first =0
							else:
								pattern[i]=''
							pattern[i+1] = pattern[i+1] + '-'+ key.split('-')[1]
						pattern.append(tee_wrap)
					else:
						index=keys[key][0]
						pattern[index] = key+'-'+str(sp['id'])
						pattern[index+1]=pattern[index+1] + '-'+ key.split('-')[1]
						#pattern.remove(pattern[i])
			#else:
				
			#print('MMMMMMMMM',sp['pattern'],'\n')			
						
		self.build_element_link()
		
		i = 0
		for	el in self.element_link_lists:
			sp = self.pattern_lists[i]
			print('MMMMMMMMM',sp['pattern'])			
			for e in el:
				print(e,':                   \t\t\t',el[e])
			i += 1

	def build_element_link(self):
		pls = self.pattern_lists
		ell= self.element_link_lists
		el_p_pool=self.el_pool
		for index in range(len(pls)):
			sp = pls[index]
			pattern = sp['pattern']
			ell.append({})
			#print(sp['id'])
			#print(sp['pattern'])
			#print(sp['args_list'])
			for el_name in pattern: 
				if el_name !='':
					if el_name not in el_p_pool:
						el = Element_link(el_name)
						el_p_pool[el_name]=el
						ell[index][el_name] = el
					else:
						ell[index][el_name] = el_p_pool[el_name]
			#for	e in ell[index]:
				#print(e,':',ell[index][e],' ',end='')
		#for	e in el_p_pool:
		#	print(e,':',el_p_pool[e])
			
		
			
example_lists=[
        ({},{},{},[{'mux':'ts','access_out':'udp://:12345'}]),
        ({},{},{},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({},{},{},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'ts','access_out':'file://:12346'}]),
        #({},{},{},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'mp4','access_out':'http://:12346'},{'mux':'ts','access_out':'file://:12346'}]),
        #({},{'acodec':'aac','aenc':'fdkaac','channels':'2','samples':'8'},{'scodec':'dvb',},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'mp4','access_out':'udp://:12346'}]),
        #({'vcodec':'h264','venc':'x264'},{'acodec':'aac','channels':'2','aenc':'fdkaac','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        #({'vcodec':'h264','venc':'x264','fps':'25','gop':'25'},{},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'rtp','access_out':'udp://:12346'},{'mux':'mp4','access_out':'udp://:12346'}]),
        #({'vcodec':'h264','venc':'x264','fps':'25','gop':'25'},{},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'mp4','access_out':'udp://:12346'}]),
        #({'vcodec':'h264','venc':'x264','fps':'25','gop':'25'},{'acodec':'aac','aenc':'fdkaac','channels':'2','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({'vcodec':'h264','venc':'x264','fps':'25','gop':'25'},{'acodec':'aac','aenc':'fdkaac','channels':'2','samplerate':'44100','ab':'256'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'file:///tmp/gst.ts'}]),
        ({'vcodec':'h264','venc':'x264'},{'acodec':'aac','channels':'2','aenc':'fdkaac','ab':'128','samplerate':'44100'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'mp4','access_out':'file:///tmp/gst.mp4'}]),
        ({'vcodec':'h264','venc':'x264','fps':'25','gop':'25'},{'acodec':'aac','aenc':'fdkaac','channels':'2','samplerate':'44100','ab':'128'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'}]),
        #({'vcodec':'h264','venc':'x264','fps':'25','gop':'25'},{'acodec':'aac','aenc':'fdkaac','channels':'2','samples':'8'},{'scodec':'dvb','senc':'unkown','x':'1','y':'x'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'ts','access_out':'file:///mtp'}]),
        #({'vcodec':'h264','venc':'open264','fps':'25','gop':'25'},{'acodec':'aac','aenc':'fdkaac','channels':'2','samples':'8'},{'scodec':'dvb','senc':'unkown','x':'1','y':'x'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'mp4','access_out':'file:///mtp'},{'mux':'mp4','access_out':'file:///mtp1'}]),
      ]

if __name__ == '__main__':
	chainner = Chain(example_lists); 
