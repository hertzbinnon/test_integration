#!/bin/bash

echo -e "\n ====== CCTV Example (Switch the scr_pipe to listen in runtime) ====== \n"

STOP=0

# Graceful cleanup upon CTRL-C
trap "STOP=1" SIGINT

# Create pipelines
echo -e "\n ====> Create the scr_pipe_1 \n"
gstd-client pipeline_create pipe_1_src  uridecodebin uri=rtmp://117.50.7.118/live/ch0 \
! "video/x-raw, framerate=30/1, width=3840, height=1920, format=I420" ! queue ! interpipesink name=src_1 \
caps=video/x-raw,width=3840,height=1920,framerate=30/1 sync=false async=false

echo -e "\n ====> Create the scr_pipe_2 \n"
gstd-client pipeline_create pipe_2_src uridecodebin uri=rtmp://117.50.7.118/live/ch1 \
! "video/x-raw, framerate=30/1, width=3840, height=1920, format=I420" ! queue ! interpipesink name=src_1 \
caps=video/x-raw,width=3840,height=1920,framerate=30/1 sync=false async=false

echo -e "\n ====> Create the sink_pipe_4 (listener) \n"
gstd-client pipeline_create pipe_4_sink interpipesrc name=interpipesrc1 listen-to=src_1 format=4 \
is-live=true allow-renegotiation=true ! videoconvert ! fpsdisplaysink

# Change pipelines to PLAYING STATE
echo -e "\n ====> Change to PLAYING STATE \n"
gstd-client pipeline_play pipe_1_src
#gstd-client pipeline_play pipe_2_src
gstd-client pipeline_play pipe_4_sink
sleep 10000

echo -e "\n ====> Every 3 seconds the sink_pipe will change the src_pipe that is listening to \n"
echo -e "\n ====> Start listening to scr_pipe_1 \n"
sleep 3

# Start alternating the source pipeline being listened
while :
do
	gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_2
	echo -e "\n ====> Change to listening to scr_pipe_2 \n"
	sleep 3
	if [ $STOP -ne 0 ]
	then
		break
	fi

	gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_3
	echo -e "\n ====> Change to listening to scr_pipe_3 \n"
	sleep 3
	if [ $STOP -ne 0 ]
	then
		break
	fi

	gstd-client element_set pipe_4_sink interpipesrc1 listen-to src_1
	echo -e "\n ====> Change to listening to scr_pipe_1 \n"
	echo -e "\n ====> Type Ctrl+C to stop the example execution, otherwise it will iterate infinitely!\n"
	sleep 3
	if [ $STOP -ne 0 ]
	then
		break
	fi
	
done

# Delete Pipelines
gstd-client pipeline_delete pipe_1_src
gstd-client pipeline_delete pipe_2_src
gstd-client pipeline_delete pipe_3_src
gstd-client pipeline_delete pipe_4_sink

echo -e "\n ====> CCTV Example Finished!!! \n"
