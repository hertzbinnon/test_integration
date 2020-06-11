#!/bin/bash
 wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch1" http://192.168.0.134:8888/postserver
sleep 3
 wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch2" http://192.168.0.134:8888/postserver
sleep 3
 wget -S -d --post-data "pull rtmp://192.168.0.134/live/ch3" http://192.168.0.134:8888/postserver
