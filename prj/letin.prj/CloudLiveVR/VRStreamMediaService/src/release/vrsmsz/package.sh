#!/bin/bash
VERSION=`grep VERSION install.sh | head -1`
VERSION=`echo $VERSION | cut -d \= -f2`
PACKAGENAME=director-$VERSION-setup.bin
ARCHIVEFILE=director.tar.gz

tar zcf $ARCHIVEFILE bin django-master nginx uploadmodule lib
cat install.sh $ARCHIVEFILE  > $PACKAGENAME
rm $ARCHIVEFILE
chmod a+x $PACKAGENAME
