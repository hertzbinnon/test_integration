#!/bin/bash

start=`date "+%s"`
for ((i=0;i<3;i++))
do
{
#	gst-launch-1.0 videotestsrc is-live=TRUE ! timeoverlay text="channel $i" ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25  ! flvmux name=muxer ! filesink location=/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/clips/ch$i.flv audiotestsrc is-live=TRUE ! audioconvert !  voaacenc ! muxer.
	nohup gst-launch-1.0 videotestsrc is-live=TRUE ! video/x-raw, width=352, height=288  ! timeoverlay text="channel $i" ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25  ! flvmux name=muxer streamable=true ! rtmp2sink location=rtmp://192.168.0.134/live/ch$i audiotestsrc is-live=TRUE wave=$i ! audioconvert !  avenc_aac perfect-timestamp=TRUE ! muxer. > /dev/null 2>&1 &
#	./gst-launch-1.0 videotestsrc is-live=TRUE  ! video/x-raw, width=1920, height=1080  ! queue ! timeoverlay text="channel 1" ! queue ! videoconvert ! queue ! x264enc byte-stream=TRUE key-int-max=30 ! queue ! flvmux name=muxer streamable=TRUE ! rtmpsink location=rtmp://10.9.91.51/live/ch7 audiotestsrc is-live=TRUE wave=1 ! queue ! audioconvert ! audio/x-raw, rate=48000, channels=2 !  queue !  avenc_aac perfect-timestamp=TRUE bitrate=128000 ! queue ! muxer.
	#./ffmpeg -re -stream_loop -1 -i /home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/4k/$i.mp4 -c copy -f flv rtmp://192.168.0.134/live/ch$i
	#nohup gst-launch-1.0 videotestsrc is-live=TRUE ! video/x-raw, width=352, height=288  ! timeoverlay text="channel $i" ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25 ! h264parse ! queue ! mpegtsmux name=muxer alignment=7 ! udpsink host=192.168.0.134 port=1234$i audiotestsrc is-live=TRUE wave=$i ! audioconvert !  avenc_aac perfect-timestamp=TRUE ! muxer. > /dev/null 2>&1 &
}&
done
#wait
end=`date "+%s"`
echo "time: `expr $end - $start`"
