#!/bin/bash
INSTALL_PATH=/tmp/ITVLT
VERSION=1.0.0-Ubuntu1804
SYSVERSION="Ubuntu 18.04.4 LTS"
HDWVERSION="GeForce GTX 960"

SYS=lsb_release -a 2> /dev/null | grep Description | cut -d : -f2 | awk '{gsub(/^\s+|\s+$/, "");print}'
HDW=nvidia-smi -q | grep "Product Name" | cut -d : -f2 | uniq |  awk '{gsub(/^\s+|\s+$/, "");print}'
if [ $SYS != $SYSVERSION ]; then
  echo "$SYS not supported !!!"
  exit 1
fi
if [ $HDW != $HDWVERSION ]; then
  echo "$HDW not supported !!!"
  exit 2
fi

sudo rm -rf /usr/local/lib64 /var/local/vrsmsz /etc/vrsmsd.conf /usr/local/bin/* /usr/local/nginx
sudo mkdir -pv /usr/local/lib64
sudo apt install python3 libx11-xcb1 libjbig0 -y
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 1
sudo apt install python3-pip -y
sudo ln -s /usr/bin/pip3 /usr/bin/pip
pip install virtualenv
sudo ln -s /home/$USER/.local/bin/virtualenv /usr/bin/virtualenv
sudo mkdir -pv /var/local/vrsmsz
sudo chown -R $USER.$USER /var/local/vrsmsz
virtualenv --python=python /var/local/vrsmsz/env
pushd  $INSTALL_PATH > /dev/null
    ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $basepath/$0`
    tail -n+$ARCHIVE $basepath/$0 | tar xz
popd  > /dev/null
if [ $? != 0 ]; then
        echo "unpack error."
        exit 1;
fi
cd $INSTALL_PATH
cp -rf django-master /var/local/vrsmsz/env
cp -rf uploadmodule /var/local/vrsmsz/env
cd /var/local/vrsmsz/env
. ./bin/activate
pip install -e django-master
deactivate
cd -
cp -rf bin/*.jpg /var/local/vrsmsz
sudo cp -f bin/*.conf /etc
sudo cp -rf bin/* /usr/local/bin/
sudo cp -rf lib/* /usr/local/lib64
sudo cp -rf  nginx /usr/local/
sudo chown -R $USER.$USER /usr/local/nginx
rm -rf $INSTALL_PATH/
exit 0;
__ARCHIVE_BELOW__
