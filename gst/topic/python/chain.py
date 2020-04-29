#!/usr/bin/python3
import os,sys
import copy

class Chain():
  """
                                                         decoding -> converter -> encoding -> muxing stream_%d
                                                        /
                               audio -> parsing -> tee1_a  
                              /                         \ 
                             /                           muxing_%d
     input -> typefind -> demux -> subts--> tee1_s
                             \                            decoding (Onlyone audio_decoder)
                              \                         / 
                               video -> parsing -> tee1_v 
                                                        \ 
                                                          muxing_%d
  """
  __track_pattern={
                      #'access':'',   #Only one
                      #'typefind':'', #unkown Only one
                      #'demuxer':'',  #unkown Only one
					  #'parser':'' ,  #unkown Only one
					  'tee1_public':'',# Only one
					  'decoder':'' , #unkown Only one
					  'converter':'', # Only one
					  'tee2_public':'',#
                      'encoder_private':'',    
                      'tee3_private':'',
                    }
  # 0 is no,1 is encode,2 is no encode,for access_out must be great than or equal to 1 
  __profile_pattern={'video':1,'audio':1,'sub':0,'access_out':1,
                      'chain':
                             {'video_track':'',
                              'audio_track':'',
                              'sub_track':'',
                              'wrapper':[]},
                       'args_list':None
                     } 
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
    self.profile_lists =[] #[{0:profile},{1:profile},...]

    self.streams=len(lists)
    stream_number=0
    for stream in lists:
      profile = copy.deepcopy(Chain.__profile_pattern)
      profile['args_list'] = stream
      self.profile_lists.append({stream_number:profile})
      if len(stream[0]) > 2:
        self.vcodecs +=1
        self.profile_lists[stream_number][stream_number]['video']=1
      else:
        if len(stream[0]) == 0:
          self.profile_lists[stream_number][stream_number]['video']=2
        else:
          self.profile_lists[stream_number][stream_number]['video']=0
          
      if len(stream[1]) > 2:
        self.acodecs +=1
        self.profile_lists[stream_number][stream_number]['audio']=1
      else:
        if len(stream[0]) == 0:
          self.profile_lists[stream_number][stream_number]['audio']=2
        else:
          self.profile_lists[stream_number][stream_number]['audio']=0
      if len(stream[2]) > 2:
        self.subdecs +=1
        self.profile_lists[stream_number][stream_number]['sub']=1
      else:
        if len(stream[0]) == 0:
          self.profile_lists[stream_number][stream_number]['sub']=2
        else:
          self.profile_lists[stream_number][stream_number]['sub']=0

      if len(stream[3]) < 1:
        return None
      self.access_outs += len(stream[3])
      self.profile_lists[stream_number][stream_number]['access_out']=len(stream[3])
      stream_number +=1

      if self.streams < self.access_outs:
        print('Wrapper may be multiplexing')

    if self.streams == 1:
      video_track_pattern = copy.deepcopy(Chain.__track_pattern)
      audio_track_pattern = copy.deepcopy(Chain.__track_pattern)
      #subs_track_pattern = copy.deepcopy(Chain.__track_pattern)
      video_track_pattern['tee1_public'] = audio_track_pattern['tee1_public'] = 'no'
      video_track_pattern['tee2_public'] = audio_track_pattern['tee2_public'] = 'no'
      if self.vcodecs == 0 and self.acodecs == 0:
        video_track_pattern['decoder'] = audio_track_pattern['decoder'] ='no'
        video_track_pattern['converter'] = audio_track_pattern['converter'] = 'no'
        video_track_pattern['encoder_private'] = audio_track_pattern['encoder_private'] = 'no'
      elif self.vcodecs == 0 and self.acodecs != 0: 
        video_track_pattern['decoder'] = 'no';    audio_track_pattern['decoder'] ='decoder'
        video_track_pattern['converter'] = 'no';    audio_track_pattern['converter'] = 'converter'
        video_track_pattern['encoder_private'] = 'no';audio_track_pattern['encoder_private'] = 'encoder_private'
      elif self.vcodecs != 0 and self.acodecs == 0: 
        video_track_pattern['decoder'] = 'decoder'; audio_track_pattern['decoder'] ='no'
        video_track_pattern['converter'] = 'converter'; audio_track_pattern['converter'] = 'no'
        video_track_pattern['encoder_private'] = 'encoder_private';audio_track_pattern['encoder_private'] = 'no'
      else:
        video_track_pattern['decoder'] = 'video_decoder'; audio_track_pattern['audio_decoder'] =''
        video_track_pattern['converter'] = 'video_converter'; audio_track_pattern['audio_converter'] = 'yes'
        video_track_pattern['encoder_private'] = 'video_encoder_private';audio_track_pattern['audio_encoder_private'] = 'yes'

      if self.profile_lists[0][0]['access_out'] > 1:
        video_track_pattern['tee3_private'] = 'video_tee3_private';
        audio_track_pattern['tee3_private'] = 'audio_tee3_private'
      else:
        video_track_pattern['tee3_private'] = audio_track_pattern['tee3_private'] = 'no'
      self.profile_lists[0][0]['chain']['video_track'] = video_track_pattern
      self.profile_lists[0][0]['chain']['audio_track'] = audio_track_pattern
      #self.profile_lists[0]['chain']['sub_track'] = subs_track_pattern
      for i in range(self.profile_lists[0][0]['access_out']):
        self.profile_lists[0][0]['chain']['wrapper'].append({'muxer_private':'muxer_%d'%i,'outer_private':'outer_%d'%i})
      print(self.profile_lists[0][0])
      return

    for profile in self.profile_lists:
      video_track_pattern = copy.deepcopy(Chain.__track_pattern)
      audio_track_pattern = copy.deepcopy(Chain.__track_pattern)
      #subs_track_pattern = copy.deepcopy(Chain.__track_pattern)

      if self.vcodecs != self.streams and self.vcodecs != 0: #
        video_track_pattern['tee1_public'] = 'video_tee1_public'
      if self.acodecs != self.streams and self.acodecs != 0: #
        audio_track_pattern['tee1_public'] = 'audio_tee1_public'

      if self.vcodecs > 1: #
        video_track_pattern['tee2_public'] = 'video_tee2_public'
      if self.acodecs > 1: #
        audio_track_pattern['tee2_public'] = 'audio_tee2_public'

      if self.vcodecs != 0:
        video_track_pattern['decoder'] = 'video_decoder'
        video_track_pattern['converter'] = 'video_converter'
      if self.acodecs != 0:
        audio_track_pattern['decoder'] = 'audio_decoder'
        video_track_pattern['converter'] = 'audio_converter'
      #if self.acodecs != self.streams and self.acodecs != 0: #
      #  audio_track_pattern['tee1_public'] = 'audio_tee1_public'

    
lists=[({'codec':'h264','fps':'25','gop':'25'},{'codec':'aac'},{'codec':'dvb'},[{'mux':'ts','access_out':'udp://:12345'},{'mux':'ts','access_out':'udp://:12346'}])]
if __name__ == '__main__':
	chainer = Chain(lists); 
