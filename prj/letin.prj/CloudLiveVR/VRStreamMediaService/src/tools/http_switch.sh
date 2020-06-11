#!/bin/bash

for ((;;))
do
{
wget -S -d --post-data "switch 0" http://192.168.0.134:8888/postserver
sleep 10
wget -S -d --post-data "switch 1" http://192.168.0.134:8888/postserver
sleep 10
wget -S -d --post-data "switch 2" http://192.168.0.134:8888/postserver
sleep 10
wget -S -d --post-data "switch 4" http://192.168.0.134:8888/postserver
sleep 10
}
done
