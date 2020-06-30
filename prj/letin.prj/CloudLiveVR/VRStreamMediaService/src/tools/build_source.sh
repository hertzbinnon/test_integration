#!/bin/bash

start=`date "+%s"`
for ((i=0;i<4;i++))
do
{
#	gst-launch-1.0 videotestsrc is-live=TRUE ! timeoverlay text="channel $i" ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25  ! flvmux name=muxer ! filesink location=/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/clips/ch$i.flv audiotestsrc is-live=TRUE ! audioconvert !  voaacenc ! muxer.
	gst-launch-1.0 videotestsrc is-live=TRUE ! video/x-raw, width=720, height=576  ! timeoverlay text="channel $i" ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25  ! flvmux name=muxer streamable=TRUE ! rtmpsink location=rtmp://192.168.0.134/live/ch$i audiotestsrc is-live=TRUE ! audioconvert !  avenc_aac perfect-timestamp=TRUE ! muxer.
	#./ffmpeg -re -stream_loop -1 -i /home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/4k/$i.mp4 -c copy -f flv rtmp://192.168.0.134/live/ch$i
}&
done
#wait
end=`date "+%s"`
echo "time: `expr $end - $start`"
