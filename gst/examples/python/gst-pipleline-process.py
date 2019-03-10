import sys
import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

stream_lists=[({'video':{}},{'audio':{}},{'subtitle':{}},[{'outaddr':'udp://:12345','iface':'eth2','wrapper':'ts'},{}]),(),(),()]
video_transform=0
audio_transform=0
streams = len(stream_lists)
for stream in stream_lists:
    if len(stream[0]['video']) == 0:
        video_transform += 1
    if len(stream[1]['audio']) == 0:
        audio_transform += 1

video_transcode= streams - video_transform  
audio_transcode= streams - audio_transform

def pad_added(src, new_pad, pipeline):
    preparser = None
    decoder = None
    sink_pad=None
    new_pad_caps = Gst.Pad.get_current_caps(new_pad)None #Gst.Caps.new_empty()
    new_pad_struct = new_pad_caps.get_structure(0)#Gst.Structure.new_empty()
    new_pad_type = new_pad_struct.get_name()
    new_pad_name = new_pad.get_name()

    print("Received new pad '%s' type '%s' \n from '%s'" % (new_pad_name,new_pad_type,src.get_name()))

    if new_pad_type == 'video/x-h265':
        preparser = Gst.element_factory_make("h265parse","preparser")
        pipeline.add(preparser)
    elif new_pad_type == 'video/x-h264':
        preparser = Gst.element_factory_make("h264parse","preparser")
        pipeline.add(preparser)
    elif new_pad_type == 'video/mpeg':
        preparser = Gst.element_factory_make("mpegvideoparse","preparser")
        pipeline.add(preparser)
    elif new_pad_type == 'audio/mpeg':
        preparser = Gst.element_factory_make("mpegvideoparse","preparser")
        pipeline.add(preparser)
    elif new_pad_type == 'audio/aac':
        preparser = Gst.element_factory_make("aacparse","preparser")
        pipeline.add(preparser)
    elif new_pad_type == 'audio/x-ac3':
        preparser = Gst.element_factory_make("ac3parse","preparser")
        pipeline.add(preparser)
    demuxer=pipeline.get_by_name('demuxer')
    demuxer.link(preparser)
    tee_layer0=None # parsing  -> tee --> decoding 
    tee_layer1=None # decoding -> tee -->encoding
    tee_layer2=None # encoding -> tee --> muxing
    stream_num = 0
    for stream in stream_lists:
        outputs = len(stream[3])
        muxer = None
        outer = None
        encoder=None
        tee_layer2=None # encoding -> tee --> muxing
        if outputs > 1 and tee_layer2 in None:
            tee_layer2 = Gst.element_factory_make('tee','teer_%d_%d' % (stream_num,stream_num))
            pipeline.add(tee_layer2)
        for out in stream[3]:
            if out['wrapper'] == 'ts':
                muxer = Gst.element_factory_make('mpegts','muxer_%d_%d' % (stream_num,stream_num))
            if out['outaddr'][:3] == 'udp':
                outer = Gst.element_factory_make('udpsrc','outer_%d_%d' % (stream_num,stream_num))
            pipeline.add(muxer)
            pipeline.add(outer)
            muxer.link(outer)
            if outputs > 1:
                tee_layer2.link(muxer)
        if video_transcode == 0 :
            preparser.link(tee_layer2)
        if audio_transcode == 0 :
            preparser.link(tee_layer2)


def pad_added(src, new_pad, pipeline):
    preparser = None
    decoder = None
    sink_pad=None
    new_pad_caps = Gst.Pad.get_current_caps(new_pad)None #Gst.Caps.new_empty()
    new_pad_struct = new_pad_caps.get_structure(0)#Gst.Structure.new_empty()
    new_pad_type = new_pad_struct.get_name()
    new_pad_name = new_pad.get_name()

    print("Received new pad '%s' type '%s' \n from '%s'" % (new_pad_name,new_pad_type,src.get_name()))
        #pad.link(self.audioconverter.get_pad("sink"))
    if streams > 1:
        if new_pad_type == 'video/x-h265':
            preparser = Gst.element_factory_make("h265parse","preparser")
            pipeline.add(preparser)
            demuxer=pipeline.get_by_name('demuxer')
            demuxer.link(preparser)
            if video_transcode != 0 and video_transform != 0:
                tee_layer0 = Gst.element_factory_make("tee","teer_layer0")
                decoder = Gst.element_factory_make('avdec_h265','decoder_layer0')
                converter = Gst.element_factory_make('autovideoconvert','converter')
                pipeline.add(tee_layer0,decoder,converter)
                decoder.link(converter)
                preparser.link(tee_layer0)
                tee_layer0.link(decoder)

                stream_num = 0
                tee_layer1=None
                tee_layer2=None
                for stream in stream_lists:
                    outputs = len(stream[3])
                    muxer = None
                    outer = None
                    encoder=None
                    tee = None
                    
                    if outputs > 1:
                        tee = Gst.element_factory_make('tee','teer_%d_%d' % (stream_num,stream_num))
                        pipeline.add(tee)
                    for out in stream[3]:
                        if out['wrapper'] == 'ts':
                            muxer = Gst.element_factory_make('mpegts','muxer_%d_%d' % (stream_num,stream_num))
                        if out['outaddr'][:3] == 'udp':
                            outer = Gst.element_factory_make('udpsrc','outer_%d_%d' % (stream_num,stream_num))
                        pipeline.add(muxer)
                        pipeline.add(outer)
                        muxer.link(outer)
                        if outputs > 1:
                            tee.link(muxer)
                    tee_layer2 = tee
                    tee = None

                    if len(stream[0]['video']) != 0 :# decoding
                        if stream[0]['video']['vcodec'] == 'h264':
                            encoder = Gst.element_factory_make('x264enc',None)
                            pipeline.add(encoder)
                        if stream[0]['video']['vcodec'] == 'h265':
                            encoder = Gst.element_factory_make('x265enc',None)
                            pipeline.add(encoder)
                        if video_transcode > 1: 
                            if tee_layer1 is None:
                                tee = Gst.element_factory_make("tee","teer_%d" %(stream_num))
                                pipeline.add(tee )
                                converter.link(tee)
                                tee.link(encoder)
                                tee_layer1=tee
                            else:
                                tee_layer1.link(encoder)
                        else:
                            converter.link(encoder)
                        encoder.link(tee_layer2)
                    else: # transforming
                        preparser.link(tee_layer2)
            elif video_tranform == 0:
                decoder = Gst.element_factory_make('avdec_h265','decoder_layer')
                tee_layer1 = Gst.element_factory_make("tee","teer_layer1")
                pipeline.add(tee_layer1,decoder)
                preparser.link(decoder)
                decoder.link(tee_layer1)
                tee_
            elif video_transcode == 0:
                tee = Gst.element_factory_make("tee","teer_layer1")
                pipeline.add(tee)
                preparser.link(tee)

    else:

def findedtype(typefinder,probability,caps,pipeline):
    size = caps.gst_size(caps)
    demuxer = None
    preparser = None
    for i in range(size):
       structure =  caps.get_structure(i)
       name = structure.get_name()
       print('find new type ',name)
       if name == 'video/mpegts':
            demuxer = Gst.element_factory_make("tsdemux","demuxer")
       elif name == 'video/quicktime':
            demuxer = Gst.element_factory_make("qtdemux","demuxer")
       elif name == 'application/x-rtp':
            demuxer = Gst.element_factory_make("qtdemux","demuxer")
    if demuxer is None:
        print('This container is not support')
        exit()
    pipeline.add(demuxer)
    demuxer=pipeline.get_by_name('typefind')
    typefind.link(demuxer)
    demuxer.connect('pad-added',pad_added,pipeline)

def build_pipeline(access_in,stream_lists):
    pipeline = Gst.Pipeline("itv-channel-1")
    access = None
    typefind = None
    if access_in == 'udp':
        access = Gst.element_factory_make("udpsrc","access")
        typefind = Gst.element_factory_make("typefind","typefinder")
        typefind.connect("have-type", findedtype, pipeline)
        #demuxer= Gst.element_factory_make("tsdemux","demuxer")
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
    for stream in stream_lists:
        pass       

    if typefind is not None:
        pipeline.add(access,typefind)
        access.link(typefind)
    pipeline.set_state(gst.STATE_PLAYING)
    return 0



if __name__ == '__main__':
    help(Gst.Caps)
    help(Gst.Structure)
    help(Gst.Pad)
    sys.exit(build_pipeline)
