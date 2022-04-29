#!/bin/bash
#VERSION=`grep VERSION install.sh | head -1`
#VERSION=`echo $VERSION | cut -d \= -f2`
#PACKAGENAME=director-$VERSION-setup.bin

PACKAGENAME=Cloud-Director-2.0.4-setup.bin
ARCHIVEFILE=director.tar.gz
DIRECTORY=/home/hertz/sdb1/cerbero/build/dist/linux_x86_64/
. Expiration
rm $PACKAGENAME
cd  program
../shc -e $DAY/$MONTH/$YEAR -m "bin.he@letinvr.com"  -r -f _daemon
../shc -e $DAY/$MONTH/$YEAR -m "bin.he@letinvr.com"  -r -f vrsmsd 
../shc -e $DAY/$MONTH/$YEAR -m "bin.he@letinvr.com"  -r -f director 
cp -f _daemon.x ../bin/_daemon
cp -f vrsmsd.x ../bin/vrsmsd
cp -f director.x ../bin/director
gcc -o vrsync -DYEAR=$YEAR -DMONTH=$MONTH -DDAY=$DAY ntptime.c
cp -f vrsync ../bin/vrsync
cd ..
Exp_time=`date -d "$YEAR-$MONTH-$DAY 00:00:00" +"%s"`
sed -i "/Exp_time=/cExp_time=$Exp_time" install.sh
./shc -e $DAY/$MONTH/$YEAR -m "bin.he@letinvr.com"  -r -f install.sh 
mv install.sh.x install.bin
#tar zcf $ARCHIVEFILE install.bin bin django-master nginx uploadmodule lib
mv bin/_vrsmsz _vrsmsz
cp $DIRECTORY/bin/vrsmsz bin/_vrsmsz
cp $DIRECTORY/bin/* bin/
mv bin/gst-launch-1.0 bin/fconv
mv bin/gstd-1.0 bin/vrsmsz
cp -rf  $DIRECTORY/lib lib
cd lib/ && ln -s gstreamer-1.0 plugins && cd -
rm -rf lib/*.a lib/*.la 
tar czf trafficserver.tar.gz usr/local
tar zcf $ARCHIVEFILE install.bin bin django-master nginx uploadmodule lib Python-3.7.5 trafficserver.tar.gz libhwloc15_2.1.0+dfsg-4_amd64.deb #python-3.7.7
cat pre-install.sh $ARCHIVEFILE  > $PACKAGENAME
mv -f _vrsmsz bin/
rm -rf $ARCHIVEFILE install.bin install.sh.x.c lib trafficserver.tar.gz
chmod a+x $PACKAGENAME
