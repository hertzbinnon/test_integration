#!/bin/bash

start=`date "+%s"`
for ((i=0;i<8;i++))
do
{
	gst-launch-1.0 videotestsrc is-live=TRUE ! timeoverlay text="channel $i" ! videoconvert ! x264enc byte-stream=TRUE key-int-max=25  ! flvmux name=muxer ! filesink location=/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/clips/ch$i.flv audiotestsrc is-live=TRUE ! audioconvert !  voaacenc ! muxer.
}&
done
wait
end=`date "+%s"`
echo "time: `expr $end - $start`"
