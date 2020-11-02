#!/bin/bash
#VERSION=`grep VERSION install.sh | head -1`
#VERSION=`echo $VERSION | cut -d \= -f2`
#PACKAGENAME=director-$VERSION-setup.bin

PACKAGENAME=Cloud-Director-1.0.0-setup.bin
ARCHIVEFILE=director.tar.gz
DIRECTORY=~/LetinCloudLive_ProgramDirectorServer/release/
. Expiration
rm Cloud-director-setup.bin
cd  program
../shc -e $DAY/$MONTH/$YEAR -r -f _daemon
../shc -e $DAY/$MONTH/$YEAR -r -f vrsmsd 
../shc -e $DAY/$MONTH/$YEAR -r -f director 
cp -f _daemon.x ../bin/_daemon
cp -f vrsmsd.x ../bin/vrsmsd
cp -f director.x ../bin/director
gcc -o vrsync -DYEAR=$YEAR -DMONTH=$MONTH -DDAY=$DAY ntptime.c
cp -f vrsync ../bin/vrsync
cd ..
Exp_time=`date -d "$YEAR-$MONTH-$DAY 00:00:00" +"%s"`
sed -i "/Exp_time=/cExp_time=$Exp_time" install.sh
./shc -e $DAY/$MONTH/$YEAR -r -f install.sh 
mv install.sh.x install.bin
#tar zcf $ARCHIVEFILE install.bin bin django-master nginx uploadmodule lib
mv bin/_vrsmsz _vrsmsz
cp $DIRECTORY/bin/_vrsmsz bin/
mv lib lib.bak
cp -rf  ~/LetinCloudLive_ProgramDirectorServer/release/lib lib
tar zcf $ARCHIVEFILE install.bin bin django-master nginx uploadmodule lib
cat pre-install.sh $ARCHIVEFILE  > $PACKAGENAME
mv -f _vrsmsz bin/
rm -rf $ARCHIVEFILE install.bin install.sh.x.c lib
mv lib.bak lib
chmod a+x $PACKAGENAME