#!/bin/bash

for ((;;))
do
{
wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"0\",\"video_id\":\"0\",\"audio_id\":\"0\",\"effect\":\"fade\",\"duration\":1}" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 3 
wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"1\",\"effect\":\"fade\",\"duration\":1}" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 3
wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"2\",\"effect\":\"fade\",\"duration\":1}" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 3
}
done
