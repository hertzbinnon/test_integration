#!/bin/bash

for ((;;))
do
{
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"0\",\"video_id\":\"0\",\"audio_id\":\"0\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/postserver  -O /dev/null
#sleep $1
wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"1\",\"video_id\":\"1\",\"audio_id\":\"-1\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/postserver  -O /dev/null
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"1\",\"effect\":\"fade\",\"duration\":1}" http://117.50.7.118:8081/postserver  -O /dev/null
sleep $1
wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"2\",\"video_id\":\"2\",\"audio_id\":\"-1\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/postserver  -O /dev/null
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"2\",\"effect\":\"fade\",\"duration\":1}" http://117.50.7.118:8081/postserver  -O /dev/null
sleep $1
wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"3\",\"video_id\":\"3\",\"audio_id\":\"-1\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/postserver  -O /dev/null
sleep $1
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"4\",\"video_id\":\"4\",\"audio_id\":\"-1\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/server  -O /dev/null
#sleep $1
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"5\",\"video_id\":\"5\",\"audio_id\":\"-1\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/server  -O /dev/null
#sleep $1
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"6\",\"video_id\":\"6\",\"audio_id\":\"-1\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/server  -O /dev/null
#sleep $1
#wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"7\",\"video_id\":\"3\",\"audio_id\":\"3\",\"effect\":\"fade\",\"duration\":1}" http://$2:8081/postserver  -O /dev/null
#sleep $1
}
done
