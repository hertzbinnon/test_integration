#!/bin/bash
PREFIX=/home/kms/
function exitif(){
	if [ $? -ne 0 ];then
		exit
	fi
}
unalias cp
mkdir -pv /home/kms/kurento-install/{lib,bin}
ln -s /home/kms/kurento-install/lib /home/kms/kurento-install/lib64
./build-tools/cmake_compile.sh kurento/kms-cmake-utils/ /home/kms/kurento-install 
exitif
cp -f build-tools/kurento-module-creator-build.sh kurento/kurento-module-creator/hertz.sh && cd kurento/kurento-module-creator && ./hertz.sh && cd -
exitif
cp -f build-tools/libs3-build.sh kurento/libs3/hertz.sh && cd kurento/libs3 && ./hertz.sh /home/kms/kurento-install && cd -

./build-tools/cmake_compile.sh kurento/usrsctp /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/jsoncpp /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-jsonrpc/ /home/kms/kurento-install

./build-tools/gst_compile.sh kurento/gstreamer /home/kms/kurento-install
./build-tools/gst_compile.sh kurento/gst-plugins-base /home/kms/kurento-install
./build-tools/gst_compile.sh kurento/gst-plugins-good /home/kms/kurento-install
./build-tools/gst_compile.sh kurento/gst-plugins-ugly /home/kms/kurento-install
./build-tools/gst_compile.sh kurento/gst-plugins-bad /home/kms/kurento-install
./build-tools/gst_compile.sh kurento/gst-libav /home/kms/kurento-install


cp build-tools/openwebrtc-gst-plugins-build.sh  kurento/openwebrtc-gst-plugins/hertz.sh && cd kurento/openwebrtc-gst-plugins && ./hertz.sh /home/kms/kurento-install && cd -

# for CodeGenerator
./build-tools/cmake_compile.sh kurento/kms-core /home/kms/kurento-install

./build-tools/cmake_compile.sh kurento/kms-elements /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-filters /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kurento-media-server/ /home/kms/kurento-install

./build-tools/cmake_compile.sh kurento/kms-chroma /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-crowddetector /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-datachannelexample /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-markerdetector /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-opencv-plugin-sample/ /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-platedetector /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-plugin-sample /home/kms/kurento-install
./build-tools/cmake_compile.sh kurento/kms-pointerdetector /home/kms/kurento-install

cp -f build-tools/libnice-build.sh kurento/libnice/hertz.sh && cd kurento/libnice && ./hertz.sh /home/kms/kurento-install && cd -
