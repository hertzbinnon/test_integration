源码版本libwetrtc.cn

webrtc源码只提供客户端功能，没有流媒体服务器功能。真正的p2p不会提供很好体验。
这套源码的别名叫做libwebrtc,(gstreamer 项目组有一篇 http://blog.nirbheek.in/2018/02/gstreamer-webrtc.html,写的非常好。)

1.  在windows/linux上同步时先安装git,python2.7,

windows上可以使用git-bash.exe 并
export DEPOT_TOOLS_WIN_TOOLCHAIN=0

export WORKSPACE=`pwd`
export CDS_CLANG_BUCKET_OVERRIDE=http://120.92.49.206:3232/chromiumsrc/commondatastorage/raw/master/public/chromium-browser-clang
export PATH=$PATH:$WORKSPACE/depot_tools
export PATH=$PATH:/D/Program\ Files/Git/usr/bin
export PATH=/D/hertz/mytools/Python27:/D/hertz/mytools/Python27/Scripts:$PATH

#!/bin/bash
git config --global url.http://120.92.49.206:3232/chromiumsrc/webrtc.git.insteadOf https://chromium.googlesource.com/external/webrtc.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/base.git.insteadOf https://chromium.googlesource.com/chromium/src/base
git config --global url.http://120.92.49.206:3232/chromiumsrc/build.git.insteadOf https://chromium.googlesource.com/chromium/src/build
git config --global url.http://120.92.49.206:3232/chromiumsrc/buildtools.git.insteadOf https://chromium.googlesource.com/chromium/src/buildtools
git config --global url.http://120.92.49.206:3232/chromiumsrc/gradle.git.insteadOf https://chromium.googlesource.com/external/github.com/gradle/gradle.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/ios.git.insteadOf https://chromium.googlesource.com/chromium/src/ios.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/testing.git.insteadOf https://chromium.googlesource.com/chromium/src/testing
git config --global url.http://120.92.49.206:3232/chromiumsrc/third_party.git.insteadOf https://chromium.googlesource.com/chromium/src/third_party
git config --global url.http://120.92.49.206:3232/chromiumsrc/clang-format.git.insteadOf https://chromium.googlesource.com/chromium/llvm-project/cfe/tools/clang-format.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libcxx.git.insteadOf https://chromium.googlesource.com/chromium/llvm-project/libcxx.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libcxxabi.git.insteadOf https://chromium.googlesource.com/chromium/llvm-project/libcxxabi.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libunwind.git.insteadOf https://chromium.googlesource.com/external/llvm.org/libunwind.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/android_ndk.git.insteadOf https://chromium.googlesource.com/android_ndk.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/android_tools.git.insteadOf https://chromium.googlesource.com/android_tools.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/auto.git.insteadOf https://chromium.googlesource.com/external/github.com/google/auto.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/catapult.git.insteadOf https://chromium.googlesource.com/catapult.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/compact_enc_det.git.insteadOf https://chromium.googlesource.com/external/github.com/google/compact_enc_det.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/colorama.git.insteadOf https://chromium.googlesource.com/external/colorama.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/depot_tools.git.insteadOf https://chromium.googlesource.com/chromium/tools/depot_tools.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/errorprone.git.insteadOf https://chromium.googlesource.com/chromium/third_party/errorprone.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/ffmpeg.git.insteadOf https://chromium.googlesource.com/chromium/third_party/ffmpeg.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/findbugs.git.insteadOf https://chromium.googlesource.com/chromium/deps/findbugs.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/freetype2.git.insteadOf https://chromium.googlesource.com/chromium/src/third_party/freetype2.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/harfbuzz.git.insteadOf https://chromium.googlesource.com/external/github.com/harfbuzz/harfbuzz.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/gtest-parallel.git.insteadOf https://chromium.googlesource.com/external/github.com/google/gtest-parallel
git config --global url.http://120.92.49.206:3232/chromiumsrc/googletest.git.insteadOf https://chromium.googlesource.com/external/github.com/google/googletest.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/icu.git.insteadOf https://chromium.googlesource.com/chromium/deps/icu.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/jsr-305.git.insteadOf https://chromium.googlesource.com/external/jsr-305.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/jsoncpp.git.insteadOf https://chromium.googlesource.com/external/github.com/open-source-parsers/jsoncpp.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/junit.git.insteadOf https://chromium.googlesource.com/external/junit.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/fuzzer.git.insteadOf https://chromium.googlesource.com/chromium/llvm-project/compiler-rt/lib/fuzzer.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libjpeg_turbo.git.insteadOf https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libsrtp.git.insteadOf https://chromium.googlesource.com/chromium/deps/libsrtp.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libvpx.git.insteadOf https://chromium.googlesource.com/webm/libvpx.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/libyuv.git.insteadOf https://chromium.googlesource.com/libyuv/libyuv.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/linux-syscall-support.git.insteadOf https://chromium.googlesource.com/linux-syscall-support.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/mockito.git.insteadOf https://chromium.googlesource.com/external/mockito/mockito.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/nasm.git.insteadOf https://chromium.googlesource.com/chromium/deps/nasm.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/openh264.git.insteadOf https://chromium.googlesource.com/external/github.com/cisco/openh264
git config --global url.http://120.92.49.206:3232/chromiumsrc/requests.git.insteadOf https://chromium.googlesource.com/external/github.com/kennethreitz/requests.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/robolectric.git.insteadOf https://chromium.googlesource.com/external/robolectric.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/ub-uiautomator.git.insteadOf https://chromium.googlesource.com/chromium/third_party/ub-uiautomator.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/usrsctp.git.insteadOf https://chromium.googlesource.com/external/github.com/sctplab/usrsctp
git config --global url.http://120.92.49.206:3232/chromiumsrc/binaries.git.insteadOf https://chromium.googlesource.com/chromium/deps/yasm/binaries.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/patched-yasm.git.insteadOf https://chromium.googlesource.com/chromium/deps/yasm/patched-yasm.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/tools.git.insteadOf https://chromium.googlesource.com/chromium/src/tools
git config --global url.http://120.92.49.206:3232/chromiumsrc/client-py.git.insteadOf https://chromium.googlesource.com/infra/luci/client-py.git
git config --global url.http://120.92.49.206:3232/chromiumsrc/boringssl.git.insteadOf https://boringssl.googlesource.com/boringssl.git

2.编辑webrtc/.gclient
solutions = [
  { "name"        : "src",
    "url"         : "https://chromium.googlesource.com/external/webrtc.git@gitlab",
    "deps_file"   : "DEPS",
    "managed"     : True,
    "custom_deps" : {
    },
    "custom_vars": {},
  },
]
target_os = [ 'android' ] #'windows','linux'

3 执行
gclient sync

4.编译设置

4.1 android 设置
apt-get install -y software-properties-common
add-apt-repository -y ppa:openjdk-r/ppa
src/build/install-build-deps-android.sh

4.2 windows 设置
cmd --> "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
    --> D:\hertz\GitHub\webrtc\libwebrtc\src> SET PATH=D:\hertz\mytools\Python27;D:\hertz\mytools\Python27\Scripts;%PATH%;%cd%\depot_tools
src\build\gn_run_binary.py --> "#if not os.path.isabs(path):
                               #  path = './' + path"
src\third_party\yasm\run_yasm.py -->"	# Assemble.
					new_argv = [];
					for i in sys.argv:
					  new_argv.append(i)
					new_argv[1]="yasm"
					result_code = subprocess.call(new_argv[1:])
					#result_code = subprocess.call(sys.argv[1:])
					if result_code != 0:
					  sys.exit(result_code)

					# Now generate the .d file listing the dependencies. The -M option makes yasm
					# write the Makefile-style dependencies to stdout, but it seems that inhibits
					# generating any compiled output so we need to do this in a separate pass.
					# However, outputting deps seems faster than actually assembling, and yasm is
					# so fast anyway this is not a big deal.
					#
					# This guarantees proper dependency management for assembly files. Otherwise,
					# we would have to require people to manually specify the .inc files they
					# depend on in the build file, which will surely be wrong or out-of-date in
					# some cases.
					#deps = subprocess.check_output(sys.argv[1:] + ['-M'])
					deps = subprocess.check_output(new_argv[1:] + ['-M'])
				    "
5.编译
gn gen out/Release "--args=is_debug=false"


			
/*************************************************************************************************************/
(0) 实验环境：<https://raw.githubusercontent.com/havfo/Kurento-Nodejs-SIP/master/images/sipnode.png>
基本硬件：	
	一台ubuntu服务器，和一个笔记本电脑。
基本组件：
	kms   	     		流服务器
	knockplop    		webrtc信令服务器，包括客户端程序。
	kamailio          	sip信令服务器
	Kurento-Nodejs-SIP 	网关服务器	(关键，没弄懂，)
						在这个例子中说明了WEBRTC 信令服务器有一个特点，那就是转发IceCandidate地址和SDP。
						换句话Peer本身也可以被代理。这个代理可以把SDP/ice等信息重定向到其它Peer.
						网关服务器接收到SIP的SDP信息后，会把它转给KMS, 并把ICE从SDP中提取出来，也转给KMS
						如此，sip ua就和kms 构成了一对peer. ?????.
						
	WEBRTC-to-SIP     	 web服务器，提供sip客户端
	
	|----------	|							0.REGISTER(name=mcu)			|-----------------------| 
	|	rtcapp	| 						|+---------------|					|	WEBRTC-to-SIP		| 
	|----------	|						|				 V					|-----------------------|
			|		sig	|-------------------|	1.INVITE|----------	| 1.INVITE			|
			|	|-------|Kurento-Nodejs-SIP	|<----------| kamailio	|<----------|		|
			|	|		|-------------------|			|route(mcu)	|			|		|
			|	V						|				|----------	|			|		|
		|-----------|					|json-rpc		^						|		|
		|knockplop	|					V				|						|		| GET	
		|-----------|				|----------	| 		|0.REGISTER				|		|
			|						|	kms		|		|						|		|
			|						|----------	|		|						|		|
			|											|						|		V
	|----------	|										|				|---------------------------| 													
	|	rtcweb	|										+---------------|sip UAC on WEBRTC-to-SIP	|
	|----------	|														|---------------------------|
	
基本工作：
	深入理解webrtc源码。从源码编写rtcApp in C++ on local和in java/jni on Android.
	不断熟悉js及js接口。
/*************************************************************************************************************/	

(一) webrtc.org 项目源代码组织与管理。
1. 基本组成结构：
	             src
					├── android_webview
					├── apps
					├── ash
					├── base
					├── blink
					├── build
					├── build_overrides
					├── cc
					├── chrome
					├── chromecast
					├── chrome_cleaner
					├── chrome_elf
					├── chromeos
					├── cloud_print
					├── components
					├── content
					├── courgette
					├── crypto
					├── dbus
					├── device
					├── docs
					├── extensions
					├── gin
					├── google_apis
					├── google_update
					├── gpu
					├── headless
					├── infra
					├── ios
					├── ipc
					├── jingle
					├── mash
					├── media
					├── mojo
					├── native_client_sdk
					├── net
					├── notification_helper
					├── pdf
					├── ppapi
					├── printing
					├── remoting
					├── rlz
					├── sandbox
					├── services
					├── skia
					├── sql
					├── storage
					├── styleguide
					├── testing
					├── third_party
					
					├── webrtc     	# 这个目录不在原始的git储藏室里。fetch和gclient将会
									# 下载webrtc源码以及如buildtools等其它工具储藏室到src
									#  gclient 会在与src的同级目录（checkout目录）生成.gclient
									#  和 .gclient_entries。
									# 例如：
									#	webrtc-checkout      
									#					├── .gclient
									#					├── .gclient_entries
									#					└── src
									#   gn gen构建进依赖.gclient_entries来生成ninja文件。
									#   $ cat .gclient_entries
									# entries = {
												  'src': 'https://chromium.googlesource.com/external/webrtc.git',
												  'src/base': 'https://chromium.googlesource.com/chromium/src/base@1f504fa0063db2adb8404480f28b694fc385e82b',
												  'src/build': 'https://chromium.googlesource.com/chromium/src/build@7e59b44eb6cce24a88adf5dd9485612fd4eb0a97',
												  'src/buildtools': 'https://chromium.googlesource.com/chromium/buildtools.git@1dcd1bdbe93467531a50b60dbd18860803ca7be1',
												  'src/testing': 'https://chromium.googlesource.com/chromium/src/testing@78c251f6c16c343195adce9bd4d326dabfd8e602',
												  'src/third_party': 'https://chromium.googlesource.com/chromium/src/third_party@22625e4eb8dab95e271851d637770ba54cbd13bd',
												  'src/third_party/boringssl/src': 'https://boringssl.googlesource.com/boringssl.git@3120950b1e27635ee9b9d167052ce11ce9c96fd4',
												  'src/third_party/catapult': 'https://chromium.googlesource.com/external/github.com/catapult-project/catapult.git@fc9479ad5e3b6a9863ba6d8517535d9b7a8e95f8',
												  'src/third_party/colorama/src': 'https://chromium.googlesource.com/external/colorama.git@799604a1041e9b3bc5d2789ecbd7e8db2e18e6b8',
												  'src/third_party/ffmpeg': 'https://chromium.googlesource.com/chromium/third_party/ffmpeg.git@ddb09a0d5aaf6aacf846355b7629953b2496b8ea',
												  'src/third_party/gflags': 'https://chromium.googlesource.com/external/webrtc/deps/third_party/gflags@892576179b45861b53e04a112996a738309cf364',
												  'src/third_party/gflags/src': 'https://chromium.googlesource.com/external/github.com/gflags/gflags@03bebcb065c83beff83d50ae025a55a4bf94dfca',
												  'src/third_party/googletest/src': 'https://chromium.googlesource.com/external/github.com/google/googletest.git@42bc671f47b122fad36db5eccbc06868afdf7862',
												  'src/third_party/gtest-parallel': 'https://chromium.googlesource.com/external/github.com/google/gtest-parallel@2a45a8d381bf1aa14071615bc787daa524e35236',
												  'src/third_party/jsoncpp/source': 'https://chromium.googlesource.com/external/github.com/open-source-parsers/jsoncpp.git@f572e8e42e22cfcf5ab0aea26574f408943edfa4',
												  'src/third_party/libFuzzer/src': 'https://chromium.googlesource.com/chromium/llvm-project/llvm/lib/Fuzzer.git@16f5f743c188c836d32cdaf349d5d3effb8a3518',
												  'src/third_party/libjpeg_turbo': 'https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git@a1750dbc79a8792dde3d3f7d7d8ac28ba01ac9dd',
												  'src/third_party/libsrtp': 'https://chromium.googlesource.com/chromium/deps/libsrtp.git@1d45b8e599dc2db6ea3ae22dbc94a8c504652423',
												  'src/third_party/libvpx/source/libvpx': 'https://chromium.googlesource.com/webm/libvpx.git@4e16f7070354fa91c1a617ee18335e580a0b8c8c',
												  'src/third_party/libyuv': 'https://chromium.googlesource.com/libyuv/libyuv.git@7bffe5e1c54bc22daebd57003735e61693719ac6',
												  'src/third_party/lss': 'https://chromium.googlesource.com/linux-syscall-support.git@63f24c8221a229f677d26ebe8f3d1528a9d787ac',
												  'src/third_party/openh264/src': 'https://chromium.googlesource.com/external/github.com/cisco/openh264@0fd88df93c5dcaf858c57eb7892bd27763f0f0ac',
												  'src/third_party/openmax_dl': 'https://chromium.googlesource.com/external/webrtc/deps/third_party/openmax.git@7acede9c039ea5d14cf326f44aad1245b9e674a7',
												  'src/third_party/usrsctp/usrsctplib': 'https://chromium.googlesource.com/external/github.com/sctplab/usrsctp@2f6478eb8d40f1766a96b5b033ed26c0c2244589',
												  'src/third_party/yasm/source/patched-yasm': 'https://chromium.googlesource.com/chromium/deps/yasm/patched-yasm.git@7da28c6c7c6a1387217352ce02b31754deb54d2a',
												  'src/tools': 'https://chromium.googlesource.com/chromium/src/tools@842d25d3f2e1232101eb437074b3597f403404bd',
												  'src/tools/gyp': 'https://chromium.googlesource.com/external/gyp.git@eb296f67da078ec01f5e3a9ea9cdc6d26d680161',
												  'src/tools/swarming_client': 'https://chromium.googlesource.com/external/swarming.client.git@a56c2b39ca23bdf41458421a7f825ddbf3f43f28',
												  'src/buildtools/clang_format/script': 'https://chromium.googlesource.com/chromium/llvm-project/cfe/tools/clang-format.git@0653eee0c81ea04715c635dd0885e8096ff6ba6d',
												  'src/buildtools/third_party/libc++/trunk': 'https://chromium.googlesource.com/chromium/llvm-project/libcxx.git@3a07dd740be63878167a0ea19fe81869954badd7',
												  'src/buildtools/third_party/libc++abi/trunk': 'https://chromium.googlesource.com/chromium/llvm-project/libcxxabi.git@4072e8fd76febee37f60aeda76d6d9f5e3791daa',
												  'src/buildtools/third_party/libunwind/trunk': 'https://chromium.googlesource.com/external/llvm.org/libunwind.git@41f982e5887185b904a456e20dfcd58e6be6cc19',
												}
	由多个子功能“git储藏室”组成。这些子储藏室本身是独立的，没有用git submodule 来管理。
	依据将要构建的目标在编译平台、操作系统、功能上等的不同，须要的子模块源码也是不同的。
	例如，linux,android,ios分别需要不同的git模块。为此webrtc.org提供了命令行工具，用来
	完成源码的下载，配置，管理，构建，编译功能。
	
	src是项目的根目录。每一个子目录都是一个单独的项目，它们的仓库另存别处，仓库名都以src来前辍
	例如：src/base,src/third_party. 而src目录里的都是代码都是一个“同步过的版本”。
	
2. 命令行工具。
	depot_tools 其中包含了gn构建工具，gclient,fetch GIT下载工具。ninja.
	例子：
	unbuntu:
		mkdir webrtc-checkout
		cd webrtc-checkout
		fetch --nohooks webrtc
		gclient sync
		
		cd src
		gn gen out/Default --args='is_debug=false'
		ninja -C out/Default
		gn clean out/Default
	android:(这种方式包含普通checkout的源码。linux开发和android开发可以同时进行)
		mkdir webrtc-android
		cd webrtc-android
		fetch --nohooks webrtc_android
		gclient sync
		
		cd src
		gn gen out/Debug --args='target_os="android" target_cpu="arm"'
		#or gn gen out/Debug --args='target_os="android" target_cpu="arm" is_debug=false'
		#To build for ARM64: use target_cpu="arm64"
		#To build for 32-bit x86: use target_cpu="x86"
		#To build for 64-bit x64: use target_cpu="x64"

		export PATH=yourroot/src/third_party/android_tools/sdk/tools:yourroot/src/third_party/android_toolssdk/platform-tools/
		. build/android/envsetup.sh

		ninja -C out/Debug
		#or
		ninja -C out/Debug AppRTCMobile
		
		在ubuntu上执行时你必须保证所有必备份工具已经被安装。
		通常的流程不会成功，要进行两次：
		mkdir webrtc-android
		cd webrtc-android
		fetch --nohooks webrtc_android （失败后，不管它）
		gclient sync （它会下载src/build/install-build-deps.sh）
		#执行
		src/build/install-build-deps.sh 
		src/build/install-build-deps-android.sh # 安装所有工具
		rm webrtc_android/*
		fetch --nohooks webrtc_android
		gclient sync #再来一次
3. 问题。
		由于谷歌源码服务器不能访问，上述方法无法正常使用。
		使用阿里云香港地址。先把源码下到云主机上，然后再下载到本地机器上。

4.国内镜像与使用方法
		https://webrtc.org.cn/mirror/

5.开发使用方法
		libwebrtc 编译后生成的都是静态库，这些库位于out/Default/out目录下。为了方便的使用它们，头文件与库的路径务必逐一列出在编译的-L和-I上。
		另外，这些库能在比构建机器，更低的版本上使用。同时，编译自己的程序时，依赖哪些库，可以参考examples/BUILD.gn,下面是片段:
			rtc_executable("peerconnection_client") {
			testonly = true
			sources = [
			  "peerconnection/client/conductor.cc",
				...............
			]
			if (is_linux) {
			  sources += [
				"peerconnection/client/linux/main.cc",
				...............
			  ]
			  cflags = [ "-Wno-deprecated-declarations" ]
			  libs = [
				"X11",
				...............
			  ]
			  deps += [ "//build/config/linux/gtk" ]
			}
			configs += [ ":peerconnection_client_warnings_config" ]
			deps += [
			  "../api:libjingle_peerconnection_test_api",
			  "../api:video_frame_api",
			  "../base:rtc_base",
			  "../base:rtc_base_approved",
			  "../base:rtc_json",
			  "../media:rtc_media",
			  "../modules/video_capture:video_capture_module", # ':'号前为模块源目录，目录下又有一个BUILD.gn，在这个文件中又有类似的编译定义。
			  "../pc:libjingle_peerconnection",
			  "../system_wrappers:field_trial_default",
			  "../system_wrappers:metrics_default",
			  "//third_party/libyuv",
			]
		  }

		
（二） webrtc 源码
1. 目录结构：								#voice_engine(2018-01.18) 已经被分解。存放音频引擎代码。主要是控制音频的采集，处理，编解码的操作。libjingle被删除。
	webrtc/
		├── api								#WebRTC 接口层。包括 DataChannel, MediaStream, SDP相关的接口。各浏览器都是通过该接口层调用的 WebRTC。
		├── audio							#存放音频网络逻辑层相关的代码。音频数据逻辑上的发送，接收等代码。
		├── build_overrides		
		├── call							#存放的是 WebRTC “呼叫（Call）” 相关逻辑层的代码。
		├── common_audio					#存放一些音频的基本算法。包括环形队列，博利叶算法，滤波器等。
		├── common_video					#存放了视频算法相关的常用工具，如libyuv, sps/pps分析器，I420缓冲器等。
		├── data
		├── examples
		├── infra
		├── logging
		├── media							#存放（recording）媒体
		├── modules							#这个目录是 WebRTC 代码中最重要的一个目录。里面包括了音视频的采集，处理，编解码器，混音等。
			├──								#视频的渲染部分video_render(commit::b99395a54464b44adf3799f8e21b962e0da232d6 2016-04-29)已经从这里删除了。因为没有浏览器需要用到这里的渲染代码。如果使用Native API 做二次开发，需要自己写视频渲染相关的代码。
			├── audio_coding				#音频编解码
			├── audio_device				#音频采集与音频播放
			├── audio_mixer					#混音相关代码，这部分是后加的 (会议混音目录被删除了）
			├── audio_processing			#音频前后期处理
			├── bitrate_controller			#码率控制
			├── congestion_controller		#流控
			├── desktop_capture				#桌面采集
			├── include					
			├── pacing						#码率探测
			├── remote_bitrate_estimator	#远端码率估算
			├── rtp_rtcp					#rtp/rtcp协议
			├── utility
			├── video_capture				#视频采集
			├── video_coding				#视频编解码
			└── video_processing			#视频前后期处理
		├── ortc
		├── p2p					#p2p相关的
		├── pc					#存放一些业务逻辑层的代码。如 channel, session等。
		├── resources
		├── rtc_base			#存放了一些基础代码。如线程，事件，socket等相关的代码。
		├── rtc_tools			#存放了一些工具代码。如视频帧比较，I420转RGB，视频帧分析。
		├── sdk					#存放了 Android 和 IOS 层代码。如视频的采集，渲染代码都在这里。
		├── stats				#存放各种数据统计相关的类
		├── system_wrappers		#与操作系统相关的代码，如 CPU特性，原子操作，读写锁，时钟等。
		├── test
		├── tools_webrtc
		└── video				#存放视频逻辑层及视频引擎层的相关的代码。视频数据逻辑上的发送，接收等代码。
								#视频引擎层就是指如何控制视频采集，处理和编解码操作的逻辑
								
注意：video_render(commit::b99395a54464b44adf3799f8e21b962e0da232d6 2016-04-29） 这个git操作可以被视为学习gn构建系统的实用入口。

(三) 相关技术
FEC (Forward Error Correction） 中文叫前向纠错,算法详请参考gstreamer conference 2017 
NACK（Non-Acknowledge）	与 tcp 之 ACK相反，tcp是收到包后发ACK，通知发送者---收到了，nack 用于rtp over udp 这种带有序号头的包，当“不连续的”收包发生时，产生NACK,从而通知回传

(四)源文件分析：
<1> 
--- api/peerconnectioninterface.h  这个文件详细说明了webrtc 在执行流程。

类的层次：
		PeerConnectionInterface（抽象类）
			--
		PeerConnectionFactoryInterface(抽象类)
			--PeerConnectionFactory（创建PeerConnectionInterface对象） 

--- pc/peerconnectionfactory(.cc/.h) 是peerconnection的构建实现。
		注意 PeerConnectionProxy类的声明与定义是在api/peerconnectionfactoryproxy.h中使用宏来定义的。

---	pc/peerconnection(.cc/.h)		 是peerconnection类的定义实现

类说明：
	class PeerConnection : public PeerConnectionInternal,
                       public DataChannelProviderInterface,
                       public rtc::MessageHandler,
                       public sigslot::has_slots<>{
					  ......
		Initialize(....){
			...
			transport_controller_->SignalCandidatesGathered.connect(
			this, &PeerConnection::OnTransportControllerCandidatesGathered);  // @1 绑定，this是slots,
			...
		}
		
		OnTransportControllerCandidatesGathered(...){  // @2  当收集到候选时.
			...
			OnIceCandidate(std::move(candidate));     
			...
		}
		
		OnIceCandidate(...){
			...
			observer_->OnIceCandidate(candidate.get()); // 观察者由自己定义,如在example中conductor类，是将候选发给remote peer.
			...
		}
	}
    由上可知，PeerConnection类使用“信号/槽”的机制，将信号绑定到pc对象上。
