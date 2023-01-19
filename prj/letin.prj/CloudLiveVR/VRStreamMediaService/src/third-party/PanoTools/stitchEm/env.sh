# build with commit 0f399501d41ab77933677f2907f41f80ceb704d7
 sudo apt install openssh-server
 sudo apt install vim
 sudo apt install ffmpeg
 sudo apt install ffmpeg-dev
 sudo apt install ffmpeg
 sudo apt install cmake ninja
 sudo apt install cmake ninja-build
 sudo apt install git
 sudo apt install libopengl-dev
 sudo apt install libopengl-dev libglx-dev
 sudo apt install libglew-dev libglew2.1
 sudo apt install bison flex
 sudo apt install libopencv-dev
 sudo apt install libceres-dev
 sudo apt install libdrm-dev
 sudo apt install qtbase5-dev
 sudo apt install qtmultimedia5-dev
 sudo apt install qttools5-dev
 sudo apt install ffmpeg
 sudo apt install libfaac-dev libmp3lame-dev librtmp-dev libvfaad-dev libx264-dev
 sudo apt install libfaac-dev libmp3lame-dev librtmp-dev libfaad-dev libx264-dev
 sudo apt install portaudio19-dev
 sudo apt install libglfw3-dev
 sudo apt install libglm-dev
cmake -DGPU_BACKEND_CUDA=ON -DGPU_BACKEND_OPENCL=OFF -DRTMP_NVENC=ON -DDISABLE_MP4=ON  -DCMAKE_CXX_FLAGS=" -I /home/hertz/sdb1/cerbero/build/sources/linux_x86_64/gstreamer-1.0/subprojects/gst-plugins-bad/sys/nvcodec/" -G Ninja    .. && ninja

cmake -DGPU_BACKEND_CUDA=ON -DGPU_BACKEND_OPENCL=OFF -DRTMP_NVENC=ON -DDISABLE_MP4=ON  -DCMAKE_CXX_FLAGS="-I /home/hertz/sdb1/gst-build/subprojects/gst-plugins-bad/sys/nvcodec/"     -G Ninja    .. && ninja
# for private ffmpeg 
export CMAKE_INCLUDE_PATH=/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/src/third-party/PanoTools/stitchEm/ffmpeg-install/include
export CMAKE_LIBRARY_PATH=/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/src/third-party/PanoTools/stitchEm/ffmpeg-install/lib
cmake -DGPU_BACKEND_CUDA=ON -DGPU_BACKEND_OPENCL=OFF -DRTMP_NVENC=ON -DDISABLE_MP4=ON  -DCMAKE_CXX_FLAGS="-I /home/hertz/sdb1/gst-build/subprojects/gst-plugins-bad/sys/nvcodec/"     -G Ninja    .. && ninja

#
# for ffmpeg 4.4.2

PATH=$PATH:/usr/local/cuda/bin PKG_CONFIG_PATH="/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/lib/pkgconfig/:/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/src/vlc-dev/vlc/contrib/x86_64-linux-gnu/lib/pkgconfig/" CFLAGS="-I/usr/local/cuda/include" LDFLAGS="-L/usr/local/cuda/lib64" ../configure --enable-libnpp --enable-nonfree --enable-cuda --enable-libx264  --disable-libxcb --disable-libxcb-shm --disable-libxcb-xfixes --disable-libxcb-shape --disable-vaapi --enable-gpl --enable-libfdk-aac  --enable-libass --enable-shared --prefix=/home/ubuntu/sdb1/install
# debug 
PATH=$PATH:/usr/local/cuda/bin PKG_CONFIG_PATH="/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/lib/pkgconfig/:/home/hertz/sdb1/cerbero/test.hertz/gst-snippets/test_integration/prj/letin.prj/CloudLiveVR/VRStreamMediaService/src/vlc-dev/vlc/contrib/x86_64-linux-gnu/lib/pkgconfig/" CFLAGS="-I/usr/local/cuda/include" LDFLAGS="-L/usr/local/cuda/lib64" ../configure --enable-libnpp --enable-nonfree --enable-cuda --enable-libx264  --disable-libxcb --disable-libxcb-shm --disable-libxcb-xfixes --disable-libxcb-shape --disable-vaapi --enable-gpl --enable-libfdk-aac  --enable-libass --enable-shared --enable-debug=3 --disable-optimizations --disable-asm --disable-stripping --prefix=/home/ubuntu/sdb1/install

export CMAKE_INCLUDE_PATH=/home/ubuntu/sdb1/install/include
export CMAKE_LIBRARY_PATH=/home/ubuntu/sdb1/install/lib
cmake -DGPU_BACKEND_CUDA=ON -DGPU_BACKEND_OPENCL=OFF -DRTMP_NVENC=ON -DDISABLE_MP4=ON  -DCMAKE_CXX_FLAGS=" -I /home/hertz/sdb1/cerbero/build/sources/linux_x86_64/gstreamer-1.0/subprojects/gst-plugins-bad/sys/nvcodec/" -G Ninja    .. && ninja -j24

## no nvcodec
cmake -DRTMP_NVENC=OFF  -G Ninja    .. && ninja -j24
