#!/bin/bash wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch1"
http://192.168.0.134:8888/postserver sleep 3 wget -S -d --post-data "pull
rtmp://192.168.0.134/live/ch2" http://192.168.0.134:8888/postserver sleep 3
wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch3" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"pull\",\"id\":1,\"url\":\"rtmp://192.168.0.134:1935/live/ch0\"}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"0\",\"effect\":\"fade\",\"duration\":1}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"publish\",\"id\":1,\"stream_id\":\"0\",\"url\":\"rtmp://192.168.0.134:1935/live/publish\",\"encoder_params\":{\"bitrate\":10,\"width\":3840,\"height\":2160,\"fps\":25}}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"refresh\"}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"delete\",\"id\":1,\"stream_id\":\"0\"}" http://192.168.0.134:8888/postserver


wget -S -d --post-data "{\"cmd\":\"delay\",\"id\":1,\"stream_id\":\"0\",\"type\":\"all\",\"time\":-100}" http://192.168.0.134:8888/postserver
