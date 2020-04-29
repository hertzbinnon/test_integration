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
    else:
        print "yes"
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

    pipeline = Gst.parse_launch('uridecodebin name=demuxer uri=multifile://chc_segment%d.ts ! queue ! mpegvideoparse gop-split=true ! queue ! mpegtsmux name=mux alignment=7  ! udpsink  host=192.168.8.62 port=12346   demuxer. ! queue ! mpegaudioparse ! mux.')
    #pipeline = Gst.parse_launch('filesrc name=demuxer location=/tmp/chc_segment1.ts ! tsdemux name=d ! queue ! mpegvideoparse gop-split=true name=video_0_0100 ! queue ! mpegtsmux name=mux alignment=7  ! udpsink  host=192.168.8.62 port=12346  d. ! queue ! mpegaudioparse name=audio_0_0101 ! mux.')
    #pipeline = Gst.parse_launch(' multifilesrc name=demuxer location=/tmp/chc_segment%d.ts ! tsdemux name=d ! queue ! mpegvideoparse gop-split=true name=video_0_0100 ! queue ! mpegtsmux name=mux alignment=7  ! udpsink  host=192.168.8.62 port=12346  d. ! queue ! mpegaudioparse name=audio_0_0101 ! mux.')
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
    #file_list = ['file:///tmp/misc.ts','file:///tmp/chc_segment1.ts']
    file_list = ['/tmp/chc_segment2.ts','/tmp/chc_segment3.ts']
    play_number=0;
    def switch_file(loop):
        #pipeline.set_state(Gst.State.PAUSED)
        #pipeline.set_state(Gst.State.READY)
        demuxer.set_state(Gst.State.READY)
        if play_number ==0:
	        play_number == 1
        else:
	        play_number == 0
        #demuxer.set_property("uri", file_list[play_number])
        print "switch ",file_list[play_number]
        demuxer.set_property("location", file_list[play_number])
        #pipeline.set_state(Gst.State.PLAYING)
        demuxer.set_state(Gst.State.PLAYING)
        GObject.timeout_add(5*1000, switch_file, loop)

    #GObject.timeout_add(5*1000, switch_file, loop)
    pipeline.set_state(Gst.State.PLAYING)
    try:
      loop.run()
    except:
      pass
    
    # cleanup
    pipeline.set_state(Gst.State.NULL)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
