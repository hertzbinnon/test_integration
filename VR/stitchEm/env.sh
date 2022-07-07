export CMAKE_INCLUDE_PATH=/home/ubuntu/sdb1/install/include
export CMAKE_LIBRARY_PATH=/home/ubuntu/sdb1/install/lib
cmake -DGPU_BACKEND_CUDA=ON -DGPU_BACKEND_OPENCL=OFF -DRTMP_NVENC=ON -DDISABLE_MP4=ON  -DCMAKE_CXX_FLAGS=" -I /home/hertz/sdb1/cerbero/build/sources/linux_x86_64/gstreamer-1.0/subprojects/gst-plugins-bad/sys/nvcodec/" -G Ninja    .. && ninja -j24
