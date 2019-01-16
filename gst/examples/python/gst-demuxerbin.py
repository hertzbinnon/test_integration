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

    pipeline = Gst.parse_launch('uridecodebin name=demuxer uri=file:///dev/stdin ! queue ! mpegvideoparse ! mpegtsmux name=mux alignment=7 ! udpsink  host=192.168.8.62 port=12346 demuxer. ! queue ! mpegaudioparse ! mux.')
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
