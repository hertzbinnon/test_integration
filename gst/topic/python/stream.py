#!/usr/bin/env python
"""
 gst-launch-1.0 srtsrc uri=srt://127.0.0.1:8888 ! tsparse ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! fakesink
 gst-launch-1.0 videotestsrc ! x264enc ! mpegtsmux ! srtsink uri=srt://:8888

gst-launch-1.0  filesrc location=/home/hebin/Game.Of.Thrones.S01.E09.2011.1080p.mkv ! queue ! matroskademux name=demuxer ! queue ! h264parse ! queue ! mpegtsmux name=mux alignment=7 ! queue ! udpsink  host=192.168.61.26 port=12345 demuxer. ! queue ! avdec_dca ! audioconvert ! audioresample ! queue ! fdkaacenc ! audio/mpeg,channels=6 ! mux.

gst-launch-1.0 multifilesrc location=/tmp/h264+aac/%d.ts ! tsdemux name=d ! queue ! h264parse ! queue ! mpegtsmux name=mux alignment=7  ! udpsink  host=192.168.8.62 port=12346  d. ! queue ! aacparse  ! mux.

gst-launch-1.0 splitfilesrc location=/tmp/h264+aac/*.ts ! tsdemux name=d ! queue ! h264parse ! queue ! mpegtsmux name=mux alignment=7  ! udpsink  host=192.168.8.62 port=12346  d. ! queue ! aacparse  ! mux.

./build/dist/linux_x86_64/bin/gst-launch-1.0 -v -e filesrc location=/media/hertz/b901d1b9-1b63-46ca-a706-86f7401fee63/hebin/4K 体验视频.mp4 ! typefind ! queue ! qtdemux name=demuxer ! h264parse ! queue ! nvdec !  autovideoconvert ! nvh265enc bitrate=1000 ! mpegtsmux name=muxer !  filesink location=/tmp/gst.ts demuxer. ! queue ! avdec_aac ! audioconvert ! fdkaacenc ! muxer.

./build/dist/linux_x86_64/bin/gst-launch-1.0 -v -e filesrc location=/media/hertz/b901d1b9-1b63-46ca-a706-86f7401fee63/hebin/4K 体验视频.mp4 ! queue ! qtdemux name=demuxer ! h264parse ! queue ! nvdec !  autovideoconvert ! nvh265enc bitrate=1000 ! mpegtsmux name=muxer !  filesink location=/tmp/gst.ts demuxer. ! queue ! avdec_aac ! audioconvert ! fdkaacenc ! muxer.

./build/dist/linux_x86_64/bin/gst-launch-1.0 uridecodebin name=decoder uri=file:///media/hertz/b901d1b9-1b63-46ca-a706-86f7401fee63/hebin/4K体验视频.mp4 ! autovideoconvert ! nvh265enc ! mpegtsmux name=muxer !  filesink location=/tmp/gst.ts decoder. ! audioconvert ! fdkaacenc ! muxer.

gst-launch-1.0 -v -e   filesrc location=/home/hebin/2-2.ts ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! tsdemux name=d d. ! queue ! h264parse  ! splitmuxsink location=%05d.ts muxer=mpegtsmux  name=mux max-size-time=10000000000  d. ! queue ! aacparse  ! mux.audio_%u

gst-launch-1.0 splitmuxsrc location=*.ts ! filesink location=yes.ts

gst-launch-1.0 uridecodebin uri=file:///home/hebin/xwhc.ts name=de !  audioconvert ! audioresample  ! webrtcdsp experimental-agc=true ! webrtcechoprobe !  audioconvert ! audioresample ! avenc_aac  ! mpegtsmux name=muxer ! filesink location=/tmp/xwhc.ts de. !   autovideoconvert !  videoscale ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 ! queue ! muxer.
gst-launch-1.0 uridecodebin uri=file:///home/hebin/hunan2.ts !  audioconvert ! audioresample  ! webrtcdsp ! webrtcechoprobe !  audioconvert ! audioresample ! avenc_aac  ! mpegtsmux ! filesink location=/tmp/hunan.ts
ffmpeg -v quiet -re  -i /home/hebin/klg.ts -c copy -f mpegts - | gst-launch-1.0 fdsrc fd=0 ! udpsink  host=192.168.8.62 port=12346
curl http://example.com:8000/stream1.ogg ! gst-launch fdsrc fd=0 ! decodebin ! autoaudiosink
gst-launch-1.0 filesrc location=/dev/stdin ! decodebin ! autoaudiosink < /path/to/file.mp3

gst-launch-1.0 filesrc location=/home/hebin/xinyule-concat.ts ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! tsdemux name=d program-number=171 d. ! queue ! mpegvideoparse  ! mpegtsmux name=mux alignment=7  ! udpsink  host=192.168.8.62 port=12346  d. ! queue ! mpegaudioparse ! mux.
/home/hebin/.temp/cerbero/build/dist/linux_x86_64/bin/gst-launch-1.0 --gst-debug=tee:4,udpsrc:4,decodebin:4,h264parse:4  udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! tee name=teer_raw ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 !  decodebin  name=decoder  decoder. ! tee name=teer ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62  port=12349  max-bitrate=4000000 sync=true decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer. teer_raw. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! tsdemux name=d d. ! queue ! h264parse ! queue ! mpegtsmux name=mux alignment=7 ! queue ! udpsink  host=192.168.8.62 port=12348  d. ! queue ! ac3parse ! mux.

gst-launch-1.0 filesrc location=/home/hebin/xinyule_catError.ts ! curlsftpsink  location=sftp://192.168.61.23/~/  file-name=1.ts  ssh-auth-type=2 user=root passwd=1234.asd
./gst-launch-1.0 videotestsrc ! x264enc ! flvmux ! rtmpsink location=rtmp://192.168.61.28/live/my-gst ## "live" is set in nginx application section
 /home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0  -v rtmpsrc location=rtmp://localhost:1935/small/test timeout=10 ! flvdemux name=demuxer  ! queue ! hlssink2  location=hlssink-%05d.ts playlist-length=10
 /home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 filesrc location=/home/hebin/CHC/100M.mp4 ! queue   ! qtdemux name=demuxer ! flvmux name=muxer  ! rtmpsink location=rtmp://192.168.61.22:1935/small/test
/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 --gst-debug=hlssink:4 rtmpsrc location=rtmp://localhost:1935/big/test timeout=10  blocksize=4000000  ! queue ! decodebin name=decoder ! queue ! x264enc  byte-stream=TRUE bframes=3 key-int-max=25  ! queue ! mpegtsmux name=muxer  ! queue ! hlssink  location=/tmp/small/hlssink-%05d.ts  playlist-length=10  playlist-location=/tmp/small/gst.m3u8 decoder. ! queue ! avenc_aac !  queue ! muxer.

/home/cerbero/build/dist/linux_x86_64/bin/gst-launch-1.0  -v rtmpsrc location=rtmp://192.168.61.27/lanting_in/channel  timeout=10 blocksize=4000000 ! decodebin name=decoder ! queue !  queue ! x264enc  byte-stream=TRUE bframes=3 key-int-max=25 ! queue ! flvmux name=muxer  ! rtmpsink location=rtmp://192.168.61.27/hls/test

/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 concat name=vc ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 ! queue ! mpegtsmux name=muxer ! queue ! udpsink host=192.168.8.62  port=12346 concat name=ac !  avenc_aac compliance=-2 ! queue ! muxer. filesrc location=/home/hebin/CHC/CHC_frag.ts !  decodebin  name=decoder ! autovideoconvert !  videoscale ! vc. decoder.  ! audioconvert ! audioresample ! queue ! ac. filesrc location=/home/hebin/CHC/sxtv_frag.ts !  decodebin  name=decoder1 ! autovideoconvert !  videoscale ! vc. decoder1.   ! audioconvert ! audioresample ! queue ! ac.

/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 concat name=vc  ! autovideoconvert !  videoscale ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 ! queue ! mpegtsmux name=muxer ! queue ! udpsink host=192.168.8.62  port=12346  concat name=ac  ! audioconvert ! audioresample ! avenc_aac compliance=-2 ! queue ! muxer. filesrc location=/home/hebin/CHC/CHC_frag.ts !  decodebin  name=decoder ! vc.  decoder. ! ac. filesrc location=/home/hebin/CHC/sxtv_frag.ts !  decodebin  name=decoder1 ! vc.  decoder1. ! ac.

/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 concat name=vc  ! autovideoconvert !  videoscale ! video/x-raw,width=720,height=576,framerate=25/1 ! aspectratiocrop aspect-ratio=1/1 ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 ! queue ! mpegtsmux name=muxer ! queue !  hlssink  location=/tmp/small/hlssink-%05d.ts  playlist-length=10  playlist-location=/tmp/small/gst.m3u8 max-files=10  target-duration=5 concat name=ac  ! audioconvert ! audioresample ! avenc_aac compliance=-2 ! queue ! muxer. filesrc location=/home/hebin/CHC/CHC_frag.ts !  decodebin  name=decoder ! vc.  decoder. ! ac. filesrc location=/home/hebin/CHC/sxtv_frag.ts !  decodebin  name=decoder1 ! vc.  decoder1. ! ac.

/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 --gst-debug=concat:4  concat name=vc adjust-base=false  ! decodebin  name=decoder ! autovideoconvert !  videoscale ! video/x-raw,width=720,height=576,framerate=25/1 ! aspectratiocrop aspect-ratio=1/1 ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 ! queue ! mpegtsmux name=muxer ! queue !  hlssink  location=/tmp/small/hlssink-%05d.ts  playlist-length=60  playlist-location=/tmp/small/gst.m3u8 max-files=100 target-duration=5   decoder. ! queue !  audioconvert ! audioresample ! avenc_aac compliance=-2 ! queue ! muxer. filesrc location=/home/hebin/CHC/CHC_frag.ts ! vc. filesrc location=/home/hebin/CHC/CHC_frag.ts ! vc.  filesrc location=/home/hebin/CHC/CHC_frag.ts ! vc.
"""
import sys

import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

def bus_call(bus, message, loop):
    t = message.type
    if t == Gst.MessageType.EOS:
        sys.stdout.write("End-of-stream\n")
        loop.quit()
    elif t == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        sys.stderr.write("Error: %s: %s\n" % (err, debug))
        loop.quit()
    elif t == Gst.MessageType.STATE_CHANGED:
        #sys.stdout.write("stream\n")
	pass
    return True

def main(args):
    """
    if len(args) != 2:
        sys.stderr.write("usage: %s <media file or uri>\n" % args[0])
        sys.exit(1)
    """
    GObject.threads_init()
    Gst.init(None)
    print Gst.version()
    print Gst.version_string()

    """export GST_DEBUG=mpegtsmux:5  """
    #Gst.debug_set_active(True)
    #Gst.DebugCategory.set_threshold('python',5)
    #Gst.debug_set_threshold_from_string('python:5,mpegtsmux=5', False)
    prog_map=Gst.Structure.new_empty('prog-map')
    print prog_map
    prog_map.set_value('sink_65',1)
    prog_map.set_value('sink_66',1)
    prog_map.set_value('sink_67',2)
    prog_map.set_value('sink_68',2)
    print prog_map.n_fields(), prog_map.get_value('sink_67'),prog_map.has_field('sink_67')
    #exit()
        
    #chain='udpsrc uri=udp://192.168.61.22:5556 ! queue !  decodebin  name=decoder  decoder. ! deinterlace mode=1 method=2  !  queue ! autovideoconvert ! queue ! videoscale ! queue  ! video/x-raw,width=720,height=576 !  queue ! openh264enc  gop-size=25 qp-max=41 multi-thread=33 bitrate=2000000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink  host=192.168.8.62 max-bitrate=3500000  port=12349  decoder. ! queue ! audioconvert ! audioresample ! avenc_aac compliance=-2 ! queue ! muxer.'
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 !  decodebin  name=decoder  decoder. ! yadif mode=1 ! queue ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 max-bitrate=5000000  port=12349  sync=false max-bitrate=4000000 decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer.');
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! decodebin  name=decoder decoder. ! deinterlace mode=1 method=4 ! autovideoconvert ! videoscale ! video/x-raw,width=1920,height=1080 ! x264enc  byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000 ref=3 ! mpegtsmux name=muxer alignment=7 ! udpsink host=192.168.8.62 max-bitrate=5000000  port=12349 sync=true');
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! queue !  decodebin  name=decoder  decoder. ! deinterlace mode=1 method=5  ! queue ! autovideoconvert ! queue ! videoscale ! queue  ! video/x-raw,width=1920,height=1080 !  queue ! openh264enc rate-control=1 gop-size=25 qp-max=51 bitrate=3000000 ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 port=12349  decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer.');
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 !  decodebin  name=decoder  decoder. ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62  port=12349  max-bitrate=4000000 sync=true decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer.')
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! tee name=teer_raw ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 !  decodebin  name=decoder  decoder. ! tee name=teer ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62  port=12349  max-bitrate=4000000 sync=true decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer. teer_raw. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! tsdemux name=d d. ! queue ! h264parse ! queue ! mpegtsmux name=mux  alignment=7 ! queue ! udpsink  host=192.168.8.62 port=12348  d. ! queue ! ac3parse ! mux.')
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 !  decodebin  name=decoder  ! tee name=teer ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62  port=12349  max-bitrate=4000000 sync=true teer. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer. teer. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000 ! queue ! mpegtsmux name=mux  alignment=7 ! queue ! udpsink  host=192.168.8.62 port=12348  teer. ! queue !  audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! mux.')
    #pipeline = Gst.parse_launch('udpsrc uri=udp://239.100.198.103:12347 multicast-iface=eth1 ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! decodebin name=decoder  decoder. ! video/x-raw ! tee name=teer_video ! queue ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=1000 bitrate=1000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62  port=12349  max-bitrate=4000000 sync=true decoder. ! queue ! audio/x-raw ! tee name=teer_audio  ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer. teer_video. ! yadif mode=1 ! queue ! videorate drop-only=true  ! autovideoconvert !  videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 !  queue ! x264enc byte-stream=TRUE bframes=3 key-int-max=25 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=1000 bitrate=1000  ! queue ! mpegtsmux name=mux alignment=7 ! queue ! udpsink  host=192.168.8.62 port=12348  teer_audio. ! queue !  audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! mux.')
	#pipeline = Gst.parse_launch('filesrc location=/home/hebin/xinyule_catError.ts ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! tee name=teer ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! decodebin  name=decoder  program_number=171  decoder. ! yadif mode=1 ! queue !  videorate drop-only=true  ! queue ! autovideoconvert ! queue ! videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1 ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 max-bitrate=7000000  port=12349 sync=true decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer. teer. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! decodebin  name=decoder1  program_number=172  decoder1. ! autovideoconvert ! chromahold target-g=255 ! autovideoconvert !  queue !  videorate drop-only=true  ! queue ! autovideoconvert ! queue ! videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1  ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000 ! queue ! muxer.  decoder1. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer.')
    #pipeline = Gst.parse_launch( 'compositor name=comp sink_1::alpha=0.5 sink_1::xpos=50 sink_1::ypos=50 ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 max-bitrate=7000000  port=12349 sync=true  videotestsrc pattern=snow timestamp-offset=3000000000 ! video/x-raw,format=AYUV,width=640,height=480,framerate=(fraction)30/1 ! timeoverlay ! queue2 ! comp. videotestsrc pattern=smpte ! video/x-raw,format=AYUV,width=800,height=600,framerate=(fraction)10/1 ! timeoverlay ! queue2 ! comp.')
    #pipeline = Gst.parse_launch(' mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 port=12349 sync=true async=false  concat name=c ! avenc_aac compliance=-2 ! queue ! muxer.  audiotestsrc num-buffers=500 ! audio/x-raw ! c. audiotestsrc num-buffers=1000  ! audio/x-raw ! c.  concat name=c1 ! x264enc byte-stream=TRUE key-int-max=30 ! queue ! muxer.   videotestsrc num-buffers=500 pattern=ball ! video/x-raw,width=320,height=240,framerate=(fraction)30/1 ! c1.  videotestsrc num-buffers=1000 pattern=snow ! video/x-raw,width=320,height=240,framerate=(fraction)30/1 ! c1.')
    #pipeline = Gst.parse_launch('filesrc location=/home/hebin/xinyule_catError.ts ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! tee name=teer ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! decodebin  name=decoder  program_number=171  decoder. ! yadif mode=1 ! queue !  videorate drop-only=true  ! queue ! autovideoconvert ! queue ! videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1  ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 max-bitrate=7000000  port=12349 sync=true decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer. teer. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! decodebin  name=decoder1  program_number=172  decoder1. ! yadif mode=1 ! queue !  videorate drop-only=true  ! queue ! autovideoconvert ! queue ! videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1  ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000 ! queue ! muxer.  decoder1. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac compliance=-2 ! queue ! muxer.')
    pipeline = Gst.parse_launch('filesrc location=/home/hebin/xinyule_catError.ts ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! tee name=teer ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! decodebin  name=decoder  program_number=171  decoder. ! yadif mode=1 ! queue !  videorate drop-only=true  ! queue ! autovideoconvert ! queue ! faceblur ! queue ! autovideoconvert  ! videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1  ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000  ! queue ! mpegtsmux name=muxer alignment=7 ! queue ! udpsink host=192.168.8.62 max-bitrate=7000000  port=12346 sync=true decoder. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac ! queue ! muxer. teer. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! decodebin  name=decoder1  program_number=172  decoder1. ! yadif mode=1 ! queue !  videorate drop-only=true  ! queue ! autovideoconvert ! queue ! faceblur ! queue ! autovideoconvert ! videoscale ! video/x-raw,width=1920,height=1080,framerate=25/1  ! x264enc byte-stream=TRUE key-int-max=25 bframes=3 qp-max=41 interlaced=false threads=0 vbv-buf-capacity=3000 bitrate=3000 ! queue ! muxer.  decoder1. ! queue ! audioconvert ! audioresample ! queue ! avenc_aac ! queue ! muxer.')
    if not pipeline:
        sys.stderr.write("'pipeline' gstreamer plugin missing\n")
        sys.exit(1)
    muxer=pipeline.get_by_name('muxer')
    muxer.set_property('prog-map', prog_map)
    # take the commandline argument and ensure that it is a uri
    """
    if Gst.uri_is_valid(args[1]):
      uri = args[1]
    else:
      uri = Gst.filename_to_uri(args[1])
    pipeline.set_property('uri', uri)

    # create and event loop and feed gstreamer bus mesages to it
    """
    loop = GObject.MainLoop()

    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect ("message", bus_call, loop)
    
    # start play back and listed to events
    pipeline.set_state(Gst.State.PLAYING)
    try:
      loop.run()
    except:
      pass
    
    # cleanup
    pipeline.set_state(Gst.State.NULL)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
