#!/bin/bash

PATH=/usr/local/bin:/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/bin/:$PATH:/home/hertz/sdb1/Qt/Qt-5.13.2/5.13.2/gcc_64//bin/
export GST_PLUGIN_PATH=/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/lib/gstreamer-1.0
export LD_LIBRARY_PATH=/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/lib:/home/hertz/sdb1/Qt/Qt-5.13.2/5.13.2/gcc_64/lib
export GST_OMX_CONFIG_DIR=/home/hertz/.cache/cerbero-sources/gst-omx-1.0/config/bellagio/gstomx.conf
export PKG_CONFIG_PATH=/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/lib/pkgconfig/:/home/hertz/sdb1/Qt/Qt-5.13.2/5.13.2/gcc_64/lib/pkgconfig

#/proc/driver/nvidia/version
NVIDIA_KERNEL_DRIVER_VERSION="NVRM version: NVIDIA UNIX x86_64 Kernel Module  435.21  Sun Aug 25 08:17:57 CDT 2019
                      GCC version:  gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)"
LINUX_KERNEL_VERSION="Linux version 5.3.0-56-generic (buildd@lcy01-amd64-017) (gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)) #50~18.04.1-Ubuntu SMP Wed May 27 08:52:12 UTC 2020"
CUDA_VERSION=10.1
NVIDIA_SMI=435.21
NVIDIA_DRIVER_VERSION=435.21
export PKG_CONFIG_PATH=`pwd`/libsoap-1.1.0/install/lib/pkgconfig/:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=`pwd`/libsoap-1.1.0/install/lib/:$LD_LIBRARY_PATH
