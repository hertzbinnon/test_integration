#!/bin/bash
sudo rm -rf /usr/local/lib64 /var/local/vrsmsz /etc/vrsmsd.conf /usr/local/bin/* /usr/local/nginx
sudo mkdir -pv /usr/local/lib64
sudo apt install python3 libx11-xcb1 libjbig0
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 1
sudo apt install python3-pip -y
sudo ln -s /usr/bin/pip3 /usr/bin/pip
pip install virtualenv
sudo ln -s /home/$USER/.local/bin/virtualenv /usr/bin/virtualenv
sudo mkdir -pv /var/local/vrsmsz
sudo chown -R $USER.$USER /var/local/vrsmsz
virtualenv --python=python /var/local/vrsmsz/env
cp -rf django-master /var/local/vrsmsz/env
cp -rf uploadmodule /var/local/vrsmsz/env
cd /var/local/vrsmsz/env
. ./bin/activate
pip install -e django-master
deactivate
cd -
cp -rf bin/*.jpg /var/local/vrsmsz
sudo mv -f bin/*.conf /etc
sudo cp -rf bin/* /usr/local/bin/
sudo cp -rf lib/* /usr/local/lib64
sudo cp -rf  nginx /usr/local/
sudo chown -R $USER.$USER /usr/local/nginx

