#!/bin/bash wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch1"
http://192.168.0.134:8888/postserver sleep 3 wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch2" http://192.168.0.134:8888/postserver sleep 3
wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch3" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"pull\",\"id\":1,\"url\":\"rtmp://192.168.0.134:1935/live/ch0\"}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"switch\",\"id\":0,\"stream_id\":\"0\",\"effect\":\"fade\",\"duration\":1}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"publish\",\"id\":1,\"stream_id\":\"0\",\"url\":\"rtmp://192.168.0.134:1935/live/publish\",\"encoder_params\":{\"bitrate\":10,\"width\":3840,\"height\":2160,\"fps\":25}}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"refresh\"}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"delete\",\"id\":1,\"stream_id\":\"0\"}" http://192.168.0.134:8888/postserver


wget -S -d --post-data "{\"cmd\":\"delay\",\"id\":1,\"stream_id\":\"0\",\"type\":\"all\",\"time\":-100}" http://192.168.0.134:8888/postserver

wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"add\",\"render_id\":-1,\"logo_params\":[{\"pathname\":\"/tmp/image.jpg\",\"rect\":{\"left\":100,\"top\":100,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver
wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"delete\",\"render_id\":0,\"logo_params\":[{\"pathname\":\"/tmp/image.jpg\",\"rect\":{\"left\":100,\"top\":100,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver


wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"add\",\"render_id\":-1,\"logo_params\":[{\"pathname\":\"render1.jpg\",\"rect\":{\"left\":100,\"top\":100,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver
wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"add\",\"render_id\":-1,\"logo_params\":[{\"pathname\":\"render2.jpg\",\"rect\":{\"left\":300,\"top\":400,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver
wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"update\",\"render_id\":0,\"logo_params\":[{\"pathname\":\"render2.jpg\",\"rect\":{\"left\":600,\"top\":700,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver
wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"update\",\"render_id\":1,\"logo_params\":[{\"pathname\":\"render2.jpg\",\"rect\":{\"left\":600,\"top\":700,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver
wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"delete\",\"render_id\":0,\"logo_params\":[{\"pathname\":\"render2.jpg\",\"rect\":{\"left\":600,\"top\":700,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver
wget -S -d --post-data "{\"cmd\":\"logo\",\"id\":0,\"stream_id\":\"0\",\"action\":\"delete\",\"render_id\":1,\"logo_params\":[{\"pathname\":\"render2.jpg\",\"rect\":{\"left\":600,\"top\":700,\"width\":100,\"height\":100},\"duration\":10}]}" http://192.168.1.32:8888/postserver

#https://www.iteye.com/blog/yztzz-2167336
curl -F "picFile=@/home/lanting/render1.jpg;filename=render1.jpg;type=image/jpeg"  http://192.168.1.32:8080/upload
