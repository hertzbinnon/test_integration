#!/bin/bash

start=`date "+%s"`
for ((i=0;i<5;i++))
do
{
./rtmp_encoder -i /data/3D打印课的总结\(4K40M\).mp4 -r 25 -f mpegts -c:v hevc_nvenc  -minrate 15M -maxrate 15M -b:v 15M  -y /tmp/ffm-$i.ts
}&
done
wait
end=`date "+%s"`
echo "time: `expr $end - $start`"
