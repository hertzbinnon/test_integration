#!/bin/bash
#gst-launch-1.0 videotestsrc pattern=ball is-live=true ! "video/x-raw, framerate=30/1, width=640, height=480, format=I420" ! queue ! interpipesink name=src_1 caps=video/x-raw,width=640,height=480,framerate=30/1 sync=false async=false   interpipesrc name=interpipesrc1 listen-to=src_1 is-live=true allow-renegotiation=true ! videoconvert ! x264enc ! h264parse ! mpegtsmux ! udpsink host=192.168.0.166 max-bitrate=7000000  port=12349 sync=true
#gst-launch-1.0 videotestsrc pattern=ball is-live=true ! "video/x-raw, framerate=30/1, width=640, height=480, format=I420" ! queue ! interpipesink name=src_1 caps=video/x-raw,width=640,height=480,framerate=30/1 sync=false async=false audiotestsrc wave=0 is-live=true ! queue ! interpipesink name=src_11 sync=false async=false interpipesrc name=interpipesrc1 listen-to=src_1 is-live=true allow-renegotiation=true ! videoconvert ! x264enc ! h264parse ! queue ! flvmux name=muxer ! rtmp2sink location=rtmp://192.168.0.134/live/chan0 sync=false interpipesrc name=interpipesrc11 listen-to=src_11 is-live=true allow-renegotiation=true ! audioconvert ! voaacenc ! aacparse ! queue ! muxer.
#gst-launch-1.0 videotestsrc pattern=ball is-live=true ! "video/x-raw, framerate=30/1, width=640, height=480, format=I420" ! queue ! interpipesink name=src_1 caps=video/x-raw,width=640,height=480,framerate=30/1 sync=false async=false   interpipesrc name=interpipesrc1 listen-to=src_1 is-live=true allow-renegotiation=true ! videoconvert ! x264enc ! h264parse ! mpegtsmux ! udpsink host=192.168.0.166 max-bitrate=7000000  port=12349 sync=false
# gst-launch-1.0 uridecodebin uri=rtmp://192.168.0.134/live/ch0 name=source ! video/x-raw, framerate=30/1, width=720, height=576, format=Y444_10LE ! queue ! interpipesink name=src_1 caps="video/x-raw,width=720,height=576,framerate=30/1" sync=false async=false forward-events=true forward-eos=true  interpipesrc name=interpipesrc1 listen-to=src_1 is-live=true allow-renegotiation=true stream-type=0 ! queue ! videoconvert ! x264enc ! h264parse ! queue ! mpegtsmux name=muxer latency=1000000000 alignment=7 ! queue ! udpsink host=192.168.0.166 port=12349 sync=false async=false
echo -e "\n ====== CCTV Example (Switch the scr_pipe to listen in runtime) ====== \n"

STOP=0

# Graceful cleanup upon CTRL-C
trap "STOP=1" SIGINT

# Create pipelines
echo -e "\n ====> Create the scr_pipe_1 \n"
gstd-client pipeline_create pipe_1_src videotestsrc pattern=ball is-live=true \
! "video/x-raw, framerate=30/1, width=640, height=480, format=I420" ! queue ! interpipesink name=src_1 \
caps=video/x-raw sync=false async=false 

gstd-client pipeline_create pipe_11_src audiotestsrc wave=0 is-live=true ! queue ! audioconvert ! voaacenc ! aacparse ! queue ! interpipesink name=src_11 caps=audio/mpeg sync=false async=false

echo -e "\n ====> Create the scr_pipe_2 \n"
gstd-client pipeline_create pipe_2_src videotestsrc pattern=snow is-live=true \
! "video/x-raw, framerate=30/1, width=640, height=480, format=I420" ! queue ! interpipesink name=src_2 \
caps=video/x-raw,width=640,height=480,framerate=30/1 sync=false async=false audiotestsrc wave=1 is-live=true \
! queue ! interpipesink name=src_22 sync=false async=false

echo -e "\n ====> Create the scr_pipe_3 \n"
gstd-client pipeline_create pipe_3_src videotestsrc pattern=smpte is-live=true \
! "video/x-raw, framerate=30/1, width=640, height=480, format=I420" ! queue ! interpipesink name=src_3 \
caps=video/x-raw,width=640,height=480,framerate=30/1 sync=false async=false audiotestsrc wave=2 is-live=true \
! queue ! interpipesink name=src_33 sync=false async=false

echo -e "\n ====> Create the sink_pipe_4 (listener) \n"
#gstd-client pipeline_create pipe_4_sink interpipesrc name=interpipesrc1 listen-to=src_1 \
#is-live=true allow-renegotiation=true ! videoconvert ! x264enc ! h264parse ! mpegtsmux ! udpsink host=192.168.0.166 max-bitrate=7000000  port=12349 sync=false

#gstd-client pipeline_create pipe_4_sink interpipesrc name=interpipesrc1 listen-to=src_1 \
#is-live=true allow-renegotiation=true ! videoconvert ! x264enc ! h264parse ! flvmux !  rtmp2sink location=rtmp://192.168.0.134/live/chan0 sync=false

#gstd-client pipeline_create pipe_4_sink interpipesrc name=interpipesrc1 listen-to=src_1 is-live=true allow-renegotiation=true ! videoconvert ! x264enc ! h264parse ! queue ! flvmux name=muxer ! rtmp2sink location=rtmp://192.168.0.134/live/chan0 sync=false interpipesrc name=interpipesrc11 listen-to=src_11 is-live=true allow-renegotiation=true ! audioconvert ! voaacenc ! aacparse ! queue ! muxer.

gstd-client pipeline_create pipe_4_sink mpegtsmux name=muxer alignment=7 ! udpsink host=192.168.0.166 port=12349 sync=false \
	interpipesrc name=interpipesrc1 listen-to=src_1 is-live=true allow-renegotiation=true format=3 stream-sync=2 ! queue ! videoconvert ! x264enc key-int-max=30 ! h264parse !  muxer. \
	interpipesrc name=interpipesrc11 listen-to=src_11 is-live=true allow-renegotiation=true format=3 stream-sync=2 ! queue ! aacparse !  muxer.

# Change pipelines to PLAYING STATE
echo -e "\n ====> Change to PLAYING STATE \n"
gstd-client pipeline_play pipe_1_src
gstd-client pipeline_play pipe_11_src
gstd-client pipeline_play pipe_2_src
gstd-client pipeline_play pipe_3_src
gstd-client pipeline_play pipe_4_sink

echo -e "\n ====> Every 3 seconds the sink_pipe will change the src_pipe that is listening to \n"
echo -e "\n ====> Start listening to scr_pipe_1 \n"
sleep 30000

# Start alternating the source pipeline being listened
while :
do
	gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_2
	#gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_22
	echo -e "\n ====> Change to listening to scr_pipe_2 \n"
	sleep 0.3
	if [ $STOP -ne 0 ]
	then
		break
	fi

	gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_3
	#gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_33
	echo -e "\n ====> Change to listening to scr_pipe_3 \n"
	sleep 0.3
	if [ $STOP -ne 0 ]
	then
		break
	fi

	gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_1
	#gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_11
	echo -e "\n ====> Change to listening to scr_pipe_1 \n"
	echo -e "\n ====> Type Ctrl+C to stop the example execution, otherwise it will iterate infinitely!\n"
	sleep 0.3
	if [ $STOP -ne 0 ]
	then
		break
	fi
	
done

# Delete Pipelines
gstd-client pipeline_delete pipe_1_src
gstd-client pipeline_delete pipe_11_src
gstd-client pipeline_delete pipe_2_src
gstd-client pipeline_delete pipe_3_src
gstd-client pipeline_delete pipe_4_sink

echo -e "\n ====> CCTV Example Finished!!! \n"
