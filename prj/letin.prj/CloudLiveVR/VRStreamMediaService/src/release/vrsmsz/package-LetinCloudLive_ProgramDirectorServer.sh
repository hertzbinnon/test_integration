#!/bin/bash
#VERSION=`grep VERSION install.sh | head -1`
#VERSION=`echo $VERSION | cut -d \= -f2`
#PACKAGENAME=director-$VERSION-setup.bin

PACKAGENAME=Cloud-Director-1.2.5-setup.bin
ARCHIVEFILE=director.tar.gz
DIRECTORY=~/sdb1/LetinCloudLive_ProgramDirectorServer.git/release/
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
cp $DIRECTORY/bin/_vrsmsz bin/
cp $DIRECTORY/bin/fconv bin/
mv lib lib.bak
cp -rf  $DIRECTORY/lib lib
tar czf trafficserver.tar.gz usr/local
tar zcf $ARCHIVEFILE install.bin bin django-master nginx uploadmodule lib Python-3.7.5 trafficserver.tar.gz libhwloc15_2.1.0+dfsg-4_amd64.deb #python-3.7.7
cat pre-install.sh $ARCHIVEFILE  > $PACKAGENAME
mv -f _vrsmsz bin/
rm -rf $ARCHIVEFILE install.bin install.sh.x.c lib trafficserver.tar.gz
mv lib.bak lib
chmod a+x $PACKAGENAME
