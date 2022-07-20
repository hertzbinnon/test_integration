/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 videotestsrc pattern=ball   ! c.sink_0 videotestsrc pattern=snow ! c.sink_1 compositor name=c  sink_0::zorder=0 sink_1::zorder=1 sink_0::crossfade-ratio=0.49  background=black !  videoconvert ! ximagesink

gst-launch-1.0 videotestsrc ! x265enc key-int-max=30 ! h265parse config-interval=30 ! avdec_h265 ! videoconvert ! autovideosink

/home/cerbero-1.14/build/dist/linux_x86_64/bin/gst-launch-1.0 compositor name=comp     sink_0::alpha=1 sink_0::xpos=0 sink_0::ypos=0     sink_1::alpha=0.5 sink_1::xpos=320 sink_1::ypos=240 !       videoconvert ! ximagesink   videotestsrc ! "video/x-raw,format=AYUV,width=320,height=240,framerate=(fraction)30/1" ! queue2 ! comp.sink_0   videotestsrc ! "video/x-raw,format=AYUV,width=320,height=240,framerate=(fraction)30/1" ! queue2 ! comp.sink_1

gst-launch-1.0 videotestsrc ! x264enc ! rtspclientsink location=rtsp://127.0.0.1:8554/test
./test-rtsp-record "( decodebin name=depay0 ! autovideosink )"
#./test-rtsp-record "( appsink )"
./test-rtsp-launch "( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )"
#./test-rtsp-launch "( appsrc ! x264enc ! rtph264pay name=pay0 pt=96 )"
gst-launch-1.0 videotestsrc ! video/x-raw ! glupload ! gloverlay location=/usr/share/pixmaps/faces/sky.jpg ! glimagesink

#  v360 in gstavdeinterlace
## cuda memory
gst-launch-1.0 -v -e rtmpsrc location=rtmp://192.168.1.211:19350/live/ch3 ! flvdemux name=demuxer ! queue name=demux ! h265parse ! queue name=parse1 ! nvh265dec ! queue name=decoder ! cudaconvert ! cudadownload ! video/x-raw,format=I420 ! queue ! avdeinterlace  mode=1 ! cudaupload ! cudaconvert ! video/x-raw'(memory:CUDAMemory)',format=NV12  ! queue name=converter ! nvh265enc preset=3 bitrate=7000 gop-size=30 ! queue name=encoder ! h265parse ! queue name=parse2 ! flvmux latency=1000000000 streamable=true name=muxer ! rtmp2sink location=rtmp://127.0.0.1/live/pub sync=false demuxer. ! aacparse ! queue ! avdec_aac ! audioconvert  ! voaacenc ! muxer.

## memory
gst-launch-1.0 -v -e rtmpsrc location=rtmp://192.168.1.211:19350/live/ch3 ! flvdemux name=demuxer ! queue name=demux ! h265parse ! queue name=parse1 ! nvh265dec ! queue name=decoder ! videoconvert ! video/x-raw,format=I420  ! queue name=v360 ! avdeinterlace  mode=1 ! queue name=conv ! videoconvert ! video/x-raw,format=NV12  ! queue name=converter ! nvh265enc preset=3 bitrate=7000 gop-size=30 ! queue name=encoder ! h265parse ! queue name=parse2 ! flvmux latency=1000000000 streamable=true name=muxer ! rtmp2sink location=rtmp://127.0.0.1/live/pub sync=false demuxer. ! aacparse ! queue ! avdec_aac ! audioconvert  ! voaacenc ! muxer.

##  memory
gst-launch-1.0 -v -e rtmpsrc location=rtmp://192.168.1.211:19350/live/ch3 ! flvdemux name=demuxer ! queue name=demux ! h265parse ! queue name=parse1 ! nvh265dec ! queue name=decoder ! videoconvert ! avdeinterlace  mode=1 ! queue name=en ! nvh265enc preset=3 bitrate=7000 gop-size=30 ! queue name=encoder ! h265parse ! queue name=parse2 ! flvmux latency=1000000000 streamable=true name=muxer ! rtmp2sink location=rtmp://127.0.0.1/live/pub sync=false demuxer. ! aacparse ! queue ! avdec_aac ! audioconvert  ! voaacenc ! muxer.

## opengl memory

gst-launch-1.0 -v -e rtmpsrc location=rtmp://192.168.1.211:19350/live/ch3 ! flvdemux name=demuxer ! queue name=demux ! h265parse ! queue name=parse1 ! nvh265dec ! queue name=decoder ! glupload ! glcolorconvert ! queue name=gl ! gldownload ! avdeinterlace  mode=1 !  queue name=en ! nvh265enc preset=3 bitrate=7000 gop-size=30 ! queue name=encoder ! h265parse ! queue name=parse2 ! flvmux latency=1000000000 streamable=true name=muxer ! rtmp2sink location=rtmp://127.0.0.1/live/pub sync=false demuxer. ! aacparse ! queue ! avdec_aac ! audioconvert  ! voaacenc ! muxer.

## 
# glcolorscale has bugs
GST_DEBUG=glvideomixer:5 gst-launch-1.0  glvideomixer name=m  sink_0::alpha=1.0 sink_1::alpha=0.1 sink_2::alpha=0.3 sink_3::alpha=0.4 sink_4::alpha=0.5 sink_5::alpha=0.6 ! queue ! nvh264enc preset=3 ! h264parse ! queue ! flvmux ! rtmp2sink location=rtmp://192.168.1.211:19350/live/pub \
        rtmp2src location=rtmp://192.168.1.211:19350/live/origin1 timeout=30 ! flvdemux name=demuxer1 demuxer1. ! queue ! h264parse config-interval=-1 ! nvh264dec ! queue ! glupload ! glcolorconvert ! queue ! interpipesink name=src_1 caps=video/x-raw'(memory:GLMemory)',width=3840,height=2160 sync=false async=false \
        rtmp2src location=rtmp://192.168.1.211:19350/live/origin2 timeout=30 ! flvdemux name=demuxer2 demuxer2. ! queue ! h264parse config-interval=-1 ! nvh264dec ! queue ! glupload ! glcolorconvert ! queue ! interpipesink name=src_2 caps=video/x-raw'(memory:GLMemory)',width=3840,height=2160 sync=false async=false \
        rtmp2src location=rtmp://192.168.1.211:19350/live/origin3 timeout=30 ! flvdemux name=demuxer3 demuxer3. ! queue ! h264parse config-interval=-1 ! nvh264dec ! queue ! glupload ! glcolorconvert ! queue ! interpipesink name=src_3 caps=video/x-raw'(memory:GLMemory)',width=3840,height=2160 sync=false async=false \
        rtmp2src location=rtmp://192.168.1.211:19350/live/origin4 timeout=30 ! flvdemux name=demuxer4 demuxer4. ! queue ! h264parse config-interval=-1 ! nvh264dec ! queue ! glupload ! glcolorconvert ! queue ! interpipesink name=src_4 caps=video/x-raw'(memory:GLMemory)',width=3840,height=2160 sync=false async=false \
        rtmp2src location=rtmp://192.168.1.211:19350/live/origin5 timeout=30 ! flvdemux name=demuxer5 demuxer5. ! queue ! h264parse config-interval=-1 ! nvh264dec ! queue ! glupload ! glcolorconvert ! queue ! interpipesink name=src_5 caps=video/x-raw'(memory:GLMemory)',width=3840,height=2160 sync=false async=false \
        rtmp2src location=rtmp://192.168.1.211:19350/live/origin6 timeout=30 ! flvdemux name=demuxer6 demuxer6. ! queue ! h264parse config-interval=-1 ! nvh264dec ! queue ! glupload ! glcolorconvert ! queue ! interpipesink name=src_6 caps=video/x-raw'(memory:GLMemory)',width=3840,height=2160 sync=false async=false \
        interpipesrc listen-to='src_1' is-live=true allow-renegotiation=true format=3 stream-sync=0 ! queue ! m. \
        interpipesrc listen-to='src_2' is-live=true allow-renegotiation=true format=3 stream-sync=0 ! queue ! m. \
        interpipesrc listen-to='src_3' is-live=true allow-renegotiation=true format=3 stream-sync=0 ! queue ! m. \
        interpipesrc listen-to='src_4' is-live=true allow-renegotiation=true format=3 stream-sync=0 ! queue ! m. \
        interpipesrc listen-to='src_5' is-live=true allow-renegotiation=true format=3 stream-sync=0 ! queue ! m. \
        interpipesrc listen-to='src_6' is-live=true allow-renegotiation=true format=3 stream-sync=0 ! queue ! m.

        videotestsrc is-live=true ! video/x-raw, format=YUY2 ,framerate=30/1,width=3840,height=2160 ! queue ! glupload ! queue ! glcolorconvert ! queue ! interpipesink name=presrc_1 caps=video/x-raw,width=3840,height=2160 sync=false async=false \

