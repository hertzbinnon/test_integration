#!/bin/sh
compile(){
	cd $1
	cd common
	#git reset --hard
	cd ..
	rm -rf build.hertz
	mkdir -pv build.hertz
	cd build.hertz
	../autogen.sh --prefix=$2 --disable-gtk-doc --enable-tests
	make -j32 && make install 
}

compile $1 $2
