#!/bin/bash
INSTALL_PATH=/tmp/.ITVLT
VERSION=1.0.0-Ubuntu1804
rm -rf $INSTALL_PATH/
if [ ! -d $INSTALL_PATH ]; then
        mkdir -p $INSTALL_PATH
fi
if [ ! -d $INSTALL_PATH ]; then
        echo "Create install path error"
        exit 1;
fi
basepath=$(cd `dirname $0`; pwd)
pushd  $INSTALL_PATH > /dev/null
    ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $basepath/$0`
    tail -n+$ARCHIVE $basepath/$0 | tar xz
popd  > /dev/null
if [ $? != 0 ]; then
        echo "unpack error."
        exit 1;
fi
cd $INSTALL_PATH
./install.bin $INSTALL_PATH
if [ $? != 0 ]; then
   echo "Failed to install !!!"
fi
exit 0;
__ARCHIVE_BELOW__
