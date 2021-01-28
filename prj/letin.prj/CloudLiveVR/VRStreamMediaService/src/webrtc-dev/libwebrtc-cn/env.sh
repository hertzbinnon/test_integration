#
#
#
#libwebrtc.cn
#  |
# ├── depot_tools --> .git/config,> url=http://120.92.49.206:3232/chromiumsrc/depot_tools.git
# ├── env.sh
# ├── git.mirror
# └── webrtc
#       ├── .cipd
#       ├── .gclient        --> "url" : "https://chromium.googlesource.com/external/webrtc.git@gitlab", 
#       |                   --> target_os = [ 'android' ] # setting only when building for android on linux not set on windows
#       ├── .gclient_entries
#       └── src
# 
# setup git-bash.exe on windows when gclient sync
export WORKSPACE=`pwd`
export CDS_CLANG_BUCKET_OVERRIDE=http://120.92.49.206:3232/chromiumsrc/commondatastorage/raw/master/public/chromium-browser-clang
export PATH=$PATH:$WORKSPACE/depot_tools
export PATH=$PATH:/D/Program\ Files/Git/usr/bin
export PATH=/D/hertz/mytools/Python27:/D/hertz/mytools/Python27/Scripts:$PATH
