#!/bin/bash
 
echo -e "\n ====== Digital Camera Example (Switch the scr_pipe to listen in runtime) ====== \n"
 
END_MSJ="!!! Digital Camera Example Finished !!!"
REC_INDEX=0
 
# Graceful cleanup upon CTRL-C
trap "gstd-client pipeline_delete cam_src_pipe; gstd-client pipeline_delete live_preview_pipe; "\
"gstd-client pipeline_delete recording_pipe; gstd-client pipeline_delete snapshot_pipe; echo -e $END_MSJ; exit" SIGINT
 
# Create pipelines
echo -e "\n ====> Create the cam_src_pipe \n"
gstd-client pipeline_create cam_src_pipe videotestsrc pattern=ball is-live=true \
! "video/x-raw, framerate=15/1, width=640, height=480, format=I420" ! queue max-size-buffers=3 leaky=downstream \
! interpipesink name=cam_src caps=video/x-raw,width=640,height=480,framerate=15/1 sync=true async=false
 
echo -e "\n ====> Create the live_preview_pipe \n"
gstd-client pipeline_create live_preview_pipe interpipesrc name=live_prev_intpsrc listen-to=cam_src \
is-live=true allow-renegotiation=true ! queue max-size-buffers=3 leaky=downstream \
! fpsdisplaysink async=false sync=false
 
echo -e "\n ====> Create the recording_pipe \n"
gstd-client pipeline_create recording_pipe interpipesrc name=rec_intpsrc listen-to=cam_src is-live=false \
allow-renegotiation=true format=time ! queue max-size-buffers=3 leaky=downstream ! x264enc \
! h264parse ! mpegtsmux ! filesink name=rec_file_sink location=/tmp/recording_$REC_INDEX.ts async=false sync=false

echo -e "\n ====> Create the snapshot_pipe \n"
gstd-client pipeline_create snapshot_pipe interpipesrc name=snap_intpsrc listen-to=cam_src num-buffers=1 \
is-live=true allow-renegotiation=true ! queue max-size-buffers=3 leaky=downstream \
! jpegenc ! multifilesink location=/tmp/snapshot_%d.jpg async=false sync=false


# Change pipelines to PLAYING STATE
echo -e "\n ====> Start cam_src_pipe and live_preview_pipe \n"
gstd-client pipeline_play cam_src_pipe
gstd-client pipeline_play live_preview_pipe
 
# Enable recording procedure
function enable_rec {
	gstd-client element_set recording_pipe rec_file_sink location /tmp/recording_$REC_INDEX.ts
	gstd-client bus_filter recording_pipe eos
	gstd-client pipeline_play recording_pipe
}
 
# Disable recording procedure
function disable_rec {
        gstd-client bus_timeout recording_pipe 1000000000
	gstd-client event_eos recording_pipe
	# wait eos
	gstd-client bus_read recording_pipe
	gstd-client pipeline_stop recording_pipe
	REC_INDEX=$(($REC_INDEX+1))
}
 
# Take snapshot procedure
function take_snap {
	gstd-client bus_filter snapshot_pipe eos
	gstd-client bus_timeout snapshot_pipe 1000000000
	gstd-client pipeline_play snapshot_pipe
	# wait eos
	gstd-client bus_read snapshot_pipe
	gstd-client pipeline_stop snapshot_pipe
}
 
 
echo -e "\n====> Press S to take a snapshot or R to start recording video for 15 seconds \n"
echo -e "====> Type Ctrl+C to stop the example execution, otherwise it will iterate infinitely!\n"
 
 
# Start requesting actions to the user
while :
do
	read -rsn1 input
 
	if [ "$input" = "s" ];
	then
		echo -e "\n====> Take snapshot! \n"
		take_snap
		echo -e "\n====> Snapshot Taken! \n"
		echo -e "\n====> Press S to take a snapshot or R to start recording video for 15 seconds \n"
		echo -e "====> Type Ctrl+C to stop the example execution, otherwise it will iterate infinitely!\n"
	fi
 
	if [ "$input" = "r" ];
	then
		enable_rec
		echo -e "\n====> Start to record video! \n"
		sleep 15
		disable_rec
		echo -e "\n====> Finish to record video! \n"
		echo -e "\n====> Press S to take a snapshot or R to start recording video for 15 seconds \n"
		echo -e "====> Type Ctrl+C to stop the example execution, otherwise it will iterate infinitely!\n"
	fi
 
done
