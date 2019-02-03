/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 videotestsrc pattern=ball   ! c.sink_0 videotestsrc pattern=snow ! c.sink_1 compositor name=c  sink_0::zorder=0 sink_1::zorder=1 sink_0::crossfade-ratio=0.49  background=black !  videoconvert ! ximagesink

gst-launch-1.0 videotestsrc ! x265enc key-int-max=30 ! h265parse config-interval=30 ! avdec_h265 ! videoconvert ! autovideosink

/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 compositor name=comp     sink_0::alpha=1 sink_0::xpos=0 sink_0::ypos=0     sink_1::alpha=0.5 sink_1::xpos=320 sink_1::ypos=240 !       videoconvert ! ximagesink   videotestsrc ! "video/x-raw,format=AYUV,width=320,height=240,framerate=(fraction)30/1" ! queue2 ! comp.sink_0   videotestsrc ! "video/x-raw,format=AYUV,width=320,height=240,framerate=(fraction)30/1" ! queue2 ! comp.sink_1

gst-launch-1.0 videotestsrc ! x264enc ! rtspclientsink location=rtsp://127.0.0.1:8554/test
./test-rtsp-record "( decodebin name=depay0 ! autovideosink )"
#./test-rtsp-record "( appsink )"
./test-rtsp-launch "( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )"
#./test-rtsp-launch "( appsrc ! x264enc ! rtph264pay name=pay0 pt=96 )"
