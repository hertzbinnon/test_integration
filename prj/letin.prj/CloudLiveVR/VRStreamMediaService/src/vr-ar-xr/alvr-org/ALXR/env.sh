## examples gcc-11
#sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
#sudo apt purge --autoremove -y gcc-11
#sudo rm -rf /etc/apt/trusted.gpg.d/ubuntu-toolchain-r_ubuntu_test.gpg
#sudo rm -rf /etc/apt/sources.list.d/ubuntu-toolchain-r-ubuntu-test-focal.list
sudo update-alternatives --config g++
sudo update-alternatives --list g++
sudo update-alternatives --query g++
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11

cargo xtask build-alxr-client --release --reproducible --fetch --bundle-ffmpeg --no-nvidia
# if building error by offline mode cause
cargo xtask build-alxr-client --release --fetch --bundle-ffmpeg --no-nvidia

sudo update-alternatives --remove gcc /usr/bin/gcc-11 
sudo update-alternatives --remove g++ /usr/bin/g++-11 
