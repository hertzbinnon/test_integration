#https://blog.csdn.net/geyichongchujianghu/article/details/124672713

cd ~/sdb1/Android/
wget https://dl.google.com/android/repository/commandlinetools-linux-8092744_latest.zip
unzip commandlinetools-linux-8092744_latest.zip
wget https://dl.google.com/android/repository/commandlinetools-win-8092744_latest.zip
/cmdline-tools/bin/sdkmanager --list --channel=0 --sdk_root=/home/hertz/sdb1/Android/SDK/
# commandtools sdkmanager downinstall
Android Studio 3.5 -> Settings -> Android SDK -> SDK Tools -> Android SDK Tools 
# android studio
SDK/tools/bin/sdkmanager --version
./cmdline-tools/bin/sdkmanager --version
./cmdline-tools/bin/sdkmanager --version --sdk_root=/home/hertz/sdb1/Android/SDK/
SDK/tools/bin/sdkmanager --list --channel=0 --sdk_root=/home/hertz/sdb1/Android/SDK/
SDK/tools/bin/sdkmanager "build-tools;31.0.0" "platforms;android-31" --sdk_root=/home/hertz/sdb1/Android/SDK/
# Install AS on ubuntu 20.04
sudo apt install openjdk-11-jdk
sudo snap install android-studio --classic
android-studio

