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

    """
    ffmpeg -v quiet -re  -i /home/hebin/klg.ts -c copy -f mpegts - | python gst-demuxerbin.py
    gst-launch-1.0 -v -e filesrc location=/tmp/klg.ts ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0  ! tsdemux name=d  d. ! queue ! mpegvideoparse  ! splitmuxsink location=%05d.ts muxer=mpegtsmux  name=mux max-size-time=10000000000  d. ! queue ! mpegaudioparse  ! mux.audio_%u
    """
    #pipeline = Gst.parse_launch('uridecodebin name=demuxer uri=file:///dev/stdin ! queue ! mpegvideoparse ! mpegtsmux name=mux alignment=7 ! udpsink  host=192.168.8.62 port=12346 demuxer. ! queue ! mpegaudioparse ! mux.')
    #pipeline = Gst.parse_launch('uridecodebin name=demuxer uri=file:///dev/stdin ! queue ! mpegvideoparse ! queue !  splitmuxsink location=%05d.mp4 name=splitter demuxer. ! queue ! mpegaudioparse ! splitter.')
    pipeline = Gst.parse_launch('uridecodebin name=demuxer uri=file:///tmp/klg.ts ! queue ! mpegvideoparse gop-split=true ! queue !  splitmuxsink location=%05d.ts  muxer=mpegtsmux name=splitter  max-size-time=10000000000 demuxer. ! queue ! mpegaudioparse ! splitter.audio_%u')
    #pipeline = Gst.parse_launch('uridecodebin name=demuxer uri=file:///dev/stdin ! queue ! h264parse ! queue ! mpegtsmux name=mux alignment=7 ! udpsink  host=192.168.8.62 port=12346 demuxer. ! queue ! ac3parse ! queue ! mux.')
    if not pipeline:
        sys.stderr.write("'pipeline' gstreamer plugin missing\n")
        sys.exit(1)
    demuxer=pipeline.get_by_name('demuxer')
    caps = Gst.Caps.from_string("video/mpeg")
    caps.append(Gst.Caps.from_string("audio/mpeg"))
    print caps.to_string()
    demuxer.set_property('caps', caps)

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
