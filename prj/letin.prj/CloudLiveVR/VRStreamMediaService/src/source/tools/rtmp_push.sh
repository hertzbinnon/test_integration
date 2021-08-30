#!/bin/bash

start=`date "+%s"`
for ((i=0;i<8;i++))
do
{
	nohup ./ffmpeg -report -re -fflags +genpts -fflags nobuffer -probesize 50 -analyzeduration 100 -stream_loop -1 -i /home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/clips/ch$i.flv -c copy -f flv rtmp://192.168.0.134:1935/live/ch$i > /dev/null 2>&1 &
}&
done
#wait
end=`date "+%s"`
echo "time: `expr $end - $start`"
