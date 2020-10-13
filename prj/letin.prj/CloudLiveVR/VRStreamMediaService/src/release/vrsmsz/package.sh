#!/bin/bash
#VERSION=`grep VERSION install.sh | head -1`
#VERSION=`echo $VERSION | cut -d \= -f2`
#PACKAGENAME=director-$VERSION-setup.bin

PACKAGENAME=director-setup.bin
ARCHIVEFILE=director.tar.gz
. Expiration

cd  program
../shc -e $DAY/$MONTH/$YEAR -r -f _daemon
../shc -e $DAY/$MONTH/$YEAR -r -f vrsmsd 
cp -f _daemon.x ../bin/director
cp -f vrsmsd.x ../bin/vrsmsd
gcc -o vrsync -DYEAR=$YEAR -DMONTH=$MONTH -DDAY=$DAY ntptime.c
cp -f vrsync ../bin/vrsync
cd ..
sed -i "/Exp_time=/cExp_time=$YEAR-$MONTH-$DAY" install.sh
shc -e $DAY/$MONTH/$YEAR -r -f install.sh 
mv install.sh.x install.bin
rm -f install.sh.x.c
exit 0
tar zcf $ARCHIVEFILE install.bin bin django-master nginx uploadmodule lib
cat pre-install.sh $ARCHIVEFILE  > $PACKAGENAME
rm $ARCHIVEFILE install.bin
chmod a+x $PACKAGENAME
