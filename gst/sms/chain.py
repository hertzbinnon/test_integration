#!/usr/bin/python3
import os,sys
import copy
"""
	gst-launch-1.0 -e -v udpsrc uri=udp://@192.168.61.26:12346 ! tee name=teer_in ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! decodebin name=decoder !  queue !  autovideoconvert ! tee name=tee_vcodec ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! x264enc !  queue ! mpegtsmux name=muxer alignment=7 ! udpsink host=192.168.61.22 port=22345 decoder. ! queue !  audioconvert ! tee name=tee_acodec ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0   ! fdkaacenc ! queue ! muxer. teer_in. !  queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  !  typefind ! tsdemux name=demuxer !  queue ! mpegvideoparse ! queue ! mpegtsmux name=muxer1 alignment=7 ! udpsink host=192.168.61.22 port=22346 demuxer. ! queue ! mpegaudioparse ! queue ! muxer1. tee_vcodec. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! x264enc !  queue ! mpegtsmux name=muxer2 alignment=7 ! udpsink host=192.168.61.22 port=22347 tee_acodec. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0   ! fdkaacenc ! queue ! muxer2.
"""
__stream_pattern=[
	'access_in',
	'tee_in',
	'typefind' ,
	'demux',
	'aprearse',
	'vprearse',
	'sprearse',
	'adecode',
	'vdecode',
	'sdecode',
	'aconvert',
	'vconvert',
	'sconvert',
	'tee_acon',
	'tee_vcon',
	'tee_scon',
	'tee_aes',#
	'tee_ves',#
	'tee_ses',#
	'aencode',    
	'vencode',
	'sencode',
	'mux',
	'tee_mux',
	'access_out',
	]
# NOTE 
class Element_link():
	def __init__(self,name=None):
		if name is None or name not isinstance(name,str) or name not in __streame_pattern.keys():
			print("must be in ",__stream_pattern) 
        self.name=name # eg. access_in
		self.caps_name=''
		self.mod_name=''
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
	self.subdecs  = 0
	self.access_outs    = 0
	self.stream_lists =[]
	self.public_set =[]

	self.streams=len(lists)
	stream_number = 0
	for stream in lists:
		profile['track_id'] = stream_number 
		self.profile_lists.append(profile)
		if len(stream[0]) > 2:
			self.vcodecs +=1
			self.profile_lists[stream_number]['video']=1
		else:
			if len(stream[0]) == 0:
		  		self.profile_lists[stream_number]['video']=2
			else:
		  		self.profile_lists[stream_number]['video']=0
		  
		if len(stream[1]) > 2:
			self.acodecs +=1
			self.profile_lists[stream_number]['audio']=1
		else:
			if len(stream[0]) == 0:
		  		self.profile_lists[stream_number]['audio']=2
			else:
		  		self.profile_lists[stream_number]['audio']=0

		if len(stream[2]) > 2:
			self.subdecs +=1
			self.profile_lists[stream_number]['sub']=1
		else:
			if len(stream[0]) == 0:
		  		self.profile_lists[stream_number]['sub']=2
			else:
		  		self.profile_lists[stream_number]['sub']=0

		if len(stream[3]) < 1:
			return None
		self.access_outs += len(stream[3])
		self.profile_lists[stream_number]['access_out'] = len(stream[3])
		stream_number +=1

		if self.streams < self.access_outs:
			print('Wrapper may be multiplexing')

		#if self.streams == 1:

		 
example_lists=[
        ({},{},{},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({},{'acodec':'aac','channels':'2','samples':'8'},{'scodec':'dvb',},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({'vcodec':'none'},{'acodec':'aac','channels':'2','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({'vcodec':'h264','fps':'25','gop':'25'},{},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({'vcodec':'h264','fps':'25','gop':'25'},{'acodec':'aac','channels':'2','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}]),
        ({'vcodec':'h264','fps':'25','gop':'25'},{'acodec':'aac','channels':'2','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'file:///tmp/gst.ts'}]),
        ({'vcodec':'none','fps':'25','gop':'25'},{'acodec':'aac','channels':'2','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'},{'mux':'mp4','access_out':'file:///tmp/gst.mp4'}]),
        ({'vcodec':'h264','fps':'25','gop':'25'},{'acodec':'aac','channels':'2','samples':'8'},{'scodec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'}]),
      ]

if __name__ == '__main__':
	chainner = Chain(example_lists); 
