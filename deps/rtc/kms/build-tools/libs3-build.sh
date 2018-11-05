#!/bin/bash

CFLAGS="-I/usr/include" && make clean && make DESTDIR=$1 install
