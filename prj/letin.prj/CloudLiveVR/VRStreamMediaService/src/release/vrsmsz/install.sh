#!/bin/bash
sudo mkdir -pv /usr/local/lib64
sudo cp -rf bin/*.conf /etc
sudo cp -rf bin/* /usr/local/bin/
sudo cp -rf lib/* /usr/local/lib64
sudo cp -rf  nginx /usr/local/
sudo chown -R $USER.$USER /usr/local/nginx

