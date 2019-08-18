https://github.com/topics/opencv?l=python

# shared libs
cmake -DCMAKE_C_FLAGS="-L/home/cerbero-1.14/build/dist/linux_x86_64/lib -Wl,-Bsymbolic -lz -ldl" -DCMAKE_CXX_FLAGS="-L/home/cerbero-1.14/build/dist/linux_x86_64/lib -Wl,-Bsymbolic -lz -ldl" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/home/ocv/installed -DOPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules -DBUILD_SHARED_LIBS=ON -DBUILD_FAT_JAVA_LIB=OFF -DBUILD_opencv_python2=ON -DBUILD_opencv_python3=ON -DBUILD_opencv_python_bindings_generator=ON  -DBUILD_EXAMPLES=ON -DWITH_CUDA=OFF -DWITH_QT=OFF -DENABLE_PRECOMPILED_HEADERS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DOPENCV_SKIP_VISIBILITY_HIDDEN=ON ../opencv  | tee -a /home/ocv/cmake.out

# shared libs
cmake -DCMAKE_C_FLAGS="-L/home/cerbero-1.14/build/dist/linux_x86_64/lib -Wl,-Bsymbolic" -DCMAKE_CXX_FLAGS="-L/home/cerbero-1.14/build/dist/linux_x86_64/lib -Wl,-Bsymbolic" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/home/ocv/installed -DOPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules -DBUILD_SHARED_LIBS=OFF -DBUILD_FAT_JAVA_LIB=ON -DBUILD_opencv_python2=ON -DBUILD_opencv_python3=ON -DBUILD_opencv_python_bindings_generator=ON  -DBUILD_EXAMPLES=ON -DWITH_CUDA=OFF -DWITH_QT=OFF -DENABLE_PRECOMPILED_HEADERS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DOPENCV_SKIP_VISIBILITY_HIDDEN=ON ../opencv  | tee -a /home/ocv/cmake.out
# configure  libtiff in cerbero  --enable-ld-version-script at cerbero

# before building gst-plugin-bad/ext/opencv
# cp fdk-aac.pc x265.pc opencv.pc /home/cerbero/build/dist/x86_64_linux/lib/pkgconfig
#  ln -s /home/ocv/installed/include/opencv2 /usr/include/opencv2
#  ln -s /home/ocv/installed/include/opencv /usr/include/opencv  

python object_detection.py  --config=/home/ai/yolo/darknet/cfg/yolov3.cfg --model=/home/ai/yolo/darknet/yolov3.weights --classes=/home/opencv/sources/opencv/samples/data/dnn/object_detection_classes_yolov3.txt --width=416 --height=416 --scale=0.00392 --input=/home/ai/yolo/darknet/data/dog.jpg --rgb
