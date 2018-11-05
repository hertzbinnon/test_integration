#!/bin/bash
./autogen.sh 
rm -rf build.hertz
mkdir -v build.hertz
cd build.hertz
# for usrsctp you must set flags
../configure --prefix=$1 CFLAGS="-I$1/include" LDFLAGS="-L$1/lib -lpthread"
make -j32 V=1 && make install

