#!/bin/sh
compile(){
	cd $1
	#cd common
	#git reset --hard
	#cd ..
	rm -rf build.hertz
	mkdir -pv build.hertz
	cd build.hertz
	#../autogen.sh --prefix=$2
	cmake .. -DCMAKE_INSTALL_PREFIX=$2 -DCMAKE_MODULE_PATH=$2/share/cmake-2.8/Modules -DCMAKE_C_FLAGS="-I$2/include -fPIC -L$2/lib"  -DCMAKE_CXX_FLAGS="-I$2/include -fPIC -L$2/lib" -DKURENTO_MODULES_DIR=$2/share/kurento/modules
	make -j32 VERBOSE=1 && make install 
}

compile $1 $2
