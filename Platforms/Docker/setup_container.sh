#!/bin/bash
# for jenkins plugins install update local dns hosts
# Note: 1. jenkins need third package which will be download from ftp server
#          if local host cant resovle ftp server name , wen can detect mannul
echo "203.178.132.80 ftp.tsukuba.wide.ad.jp" >> /etc/hosts

jenkins_http_port=18080
docker run --detach -u root --name jenkins --restart always -p $jenkins_http_port:8080 -p 50000:50000 -v /home/docker.volume/jenkins:/var/jenkins_home jenkins

gitlab_https_port=10443
gitlab_http_port=10080
gitlab_ssh_port=10022
local_host_name=""
docker run --detach \
    --hostname gitlab.example.com \
    --publish $gitlab_https_port:443 --publish $gitlab_http_port:80 --publish $gitlab_ssh_port:22 \
    --name gitlab -u root \
    --restart always \
    --volume /home/docker.volume/gitlab/config:/etc/gitlab \
    --volume /home/docker.volume/gitlab/logs:/var/log/gitlab \
    --volume /home/docker.volume/gitlab/data:/var/opt/gitlab \
    gitlab/gitlab-ce:latest


# Android OS compile
docker run \
    -i -t  -p 20022:22 \
    --hostname android.com \
    --name android-vlc -u root \
    --restart always \
    --volume /home/android.tutorials:/opt/android \
    --volume /home:/opt/android-vlc \
    ubuntu:14.04.android.vlc.stable /bin/bash

# ATS compile
docker run \
    -i -t  -p 28080:8080 \
    --hostname android.com \
    --name trafficserver -u root \
    --restart always \
    --volume /home/trafficserver:/opt/trafficserver \
    docker.io/centos /bin/bash

# IMS compile
docker run \
    -i -t  -p 25060:5060 \
    --hostname android.com \
    --name kamailio -u root \
    --restart always \
    --volume /home/kamailio:/opt/kamailio \
    docker.io/centos /bin/bash

docker pull ubuntu:16.04
docker exec -it android-vlc /bin/bash
docker inspect container_name | grep Mounts -A 20 # 查看挂载目录

docker  rm $(docker ps -a -q)
nvidia-docker run -i -t  --hostname vrsmsz-nvidia-docker --name vrsmsz-nvidia -u root --restart always --gpus all --volume /:/opt/root 5a214d77f5d7 bash
nvidia-docker run -i -t  --hostname vrsmsz-nvidia-docker --name vrsmsz-nvidia-18.04 -u root --restart always --gpus 'all,"capabilities=all"' --volume /:/opt/root 5a214d77f5d7 bash


sudo cp -rf /var/lib/docker /home/hertz/sdb1/Docker/docker-data-root
sudo vim /lib/systemd/system/docker.service
#
#ExecStart=/usr/bin/dockerd -H fd:// --containerd=/run/containerd/containerd.sock --data-root=/home/hertz/sdb1/Docker/docker-data-root/ --exec-root=/home/hertz/sdb1/Docker/docker-exec-root
#
sudo systemctl daemon-reload
sudo systemctl restart docker.service

sudo usermod -aG sudo <username>
sudo cat /etc/group | grep sudo

nvidia-docker exec -u root -it vrsmsz-nvidia-18.04 bash

# 1.ould not select device driver "" with capabilities: [[all gpu]]
sudo docker run --rm --gpus all nvidia/cuda:11.0-base nvidia-smi
# 2. 
nvidia-docker run -i -t  --hostname vrsmsz-nvidia-docker --name test -u root --restart always --gpus all  --volume /:/opt/root 58da04891421 bash

#
newgrp docker

sudo gpasswd -a $USER docker
