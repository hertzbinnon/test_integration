#!/bin/bash

for ((;;))
do
{
wget -S -d --post-data "switch 0" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 1 
wget -S -d --post-data "switch 1" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 1
wget -S -d --post-data "switch 2" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 1
wget -S -d --post-data "switch 4" http://117.50.7.118:8888/postserver  -O /dev/null
sleep 1
}
done
