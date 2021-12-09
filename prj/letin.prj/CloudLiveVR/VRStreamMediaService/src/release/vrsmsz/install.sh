#!/bin/bash
if [ $# != 1 ];then
	echo "$0 <dir>"
	exit 11
fi
INSTALL_PATH=$1
VERSION=1.0.0-Ubuntu1804
SYSVERSION="18.04"
HDWVERSION="TeslaT4"
#HDWVERSION="NVIDIAGeForceRTX3090"

sudo apt install ntpdate lsb-release libx11-xcb1 libjbig0 libegl1 -y 
sudo apt install ./libhwloc15_2.1.0+dfsg-4_amd64.deb libtcl8.6 libhwloc-dev -y
SYS=`lsb_release -a 2> /dev/null | grep Release | cut -d : -f2 | awk '{gsub(/^\s+|\s+$/, "");print}'`
HDW=`nvidia-smi -q | grep "Product Name" | cut -d : -f2 | uniq | awk '{gsub(/^\s+|\s+$/, "");print}' | sed s/[[:space:]]//g`
if [ $SYS != $SYSVERSION ]; then
  echo "$SYS not supported !!!"
  exit 1
fi
if [ $HDW != $HDWVERSION ]; then
  echo "$HDW not supported !!!"
  exit 2
fi
#sudo apt install ntpdate -y
if [ $? != 0 ];then
	echo "Please check network !!! "
       	exit 3
fi
sudo ntpdate ntp.ntsc.ac.cn
if [ $? != 0 ];then
	echo "sync date failed !!! "
       	exit 4
fi
Exp_time=1650153600
let exp_time=$Exp_time
let elps_time=`date "+%s"`

#echo "==$elps_time"
#echo "==$exp_time"

if [ $exp_time -lt $elps_time ];then
	echo "out of time !!!"
	exit 5
fi

cd $INSTALL_PATH
export LD_LIBRARY_PATH=/usr/local/lib64:/usr/local/lib
sudo rm -rf /usr/local/lib64 /var/local/vrsmsz /etc/vrsmsd.conf /usr/local/bin/* /usr/local/nginx
sudo mkdir -p /usr/local/lib64
sudo rm -rf /usr/bin/pip  /usr/bin/virtualenv 
sudo cp -rf Python-3.7.5/* /usr/local/
#sudo apt-get remove --purge python3 python3-pip -y
#sudo apt install python3 libx11-xcb1 libjbig0 -y
#sudo apt install libx11-xcb1 libjbig0 -y
sudo update-alternatives --install /usr/bin/python python /usr/local/bin/python3 3
sudo apt install python3-pip -y
sudo ln -s /usr/bin/pip3 /usr/local/bin/pip
#pip install virtualenv
#sudo ln -s /home/$USER/.local/bin/virtualenv /usr/bin/virtualenv
sudo mkdir -p /var/local/vrsmsz
sudo chown -R $USER.$USER /var/local/vrsmsz
#virtualenv --python=python /var/local/vrsmsz/env
pyvenv-3.7 /var/local/vrsmsz/env
#cd $INSTALL_PATH
cp -rf django-master /var/local/vrsmsz/env
cp -rf uploadmodule /var/local/vrsmsz/env
cd /var/local/vrsmsz/env
. ./bin/activate
pip install -e django-master
deactivate
cd - > /dev/null 2>&1
cp -rf bin/*.jpg /var/local/vrsmsz
sudo cp -f bin/*.conf /etc
sudo cp -rf bin/fconv /usr/bin/
sudo cp -rf bin/* /usr/local/bin/
sudo cp -rf lib/* /usr/local/lib64
sudo cp -rf  nginx /usr/local/
sudo chown -R $USER.$USER /usr/local/nginx
tar xzf trafficserver.tar.gz
sudo rm -rf /usr/local/var/* /usr/local/etc
sudo cp -rf  usr/local/* /usr/local/
sudo chown -R $USER.$USER /usr/local/var
sudo chown -R $USER.$USER /usr/local/etc
rm -rf $INSTALL_PATH/
rm -rf ~/.cache/gstreamer-1.0/registry.x86_64.bin
exit 0;
