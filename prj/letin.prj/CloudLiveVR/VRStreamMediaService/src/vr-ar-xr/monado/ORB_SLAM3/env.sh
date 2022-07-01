#https://blog.csdn.net/geyichongchujianghu/article/details/124293250
git clone https://github.com/UZ-SLAMLab/ORB_SLAM3.git
cd ORB_SLAM3/
opencv_version
vim CMakeLists.txt #to change OpenCV to 4.2  and delete c++1 and c++0 in CMAKE_CXX_CLFAGS
mkdir -v Datasets
cd Datasets/
wget http://robotics.ethz.ch/~asl-datasets/ijrr_euroc_mav_dataset/vicon_room1/V1_01_easy/V1_01_easy.zip
unzip V1_01_easy.zip
cd ../
mkdir -v Deps
cd Deps/
git clone --recursive https://github.com/stevenlovegrove/Pangolin.git
cd Pangolin/
git pull -v --recurse-submodules
./scripts/install_prerequisites.sh --dry-run
./scripts/install_prerequisites.sh
cmake -B build -GNinja
cmake --build build
ctest
sudo cmake --build build -t pypangolin_pip_install
sudo /usr/bin/python3.8 -m pip install --upgrade pip
cd ../../
./build.sh

./Examples/Stereo/stereo_euroc ./Vocabulary/ORBvoc.txt ./Examples/Stereo/EuRoC.yaml ./Datasets/ ./Examples/Stereo/EuRoC_TimeStamps/V101.txt 

