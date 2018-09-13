#!/bin/bash
./autogen.sh --prefix=$1
CFLAGS="-I$1/include" &&  make -j32 V=1 && make install

