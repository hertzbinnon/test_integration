#!/bin/bash
wget -t 0  http://mirrors.ustc.edu.cn/aosp-monthly/aosp-20160806.tar http://mirrors.ustc.edu.cn
curl https://storage.googleapis.com/git-repo-downloads/repo > repo
tar xf aosp-20160806.tar
chmod +x repo
#sed -i 's/https:\/\/android.googlesource.com\/platform\/manifest/git:\/\/mirrors.ustc.edu.cn\/aosp\/platform\/manifest' .repo/manifests.git/config
cd aosp 
sed -i 's/https:\/\/android.googlesource.com\/platform\/manifest/https:\/\/mirrors.ustc.edu.cn\/aosp\/platform\/manifest' .repo/manifests.git/config
../pre_repo.sh
