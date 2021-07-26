#!/bin/bash
#VERSION=`grep VERSION install.sh | head -1`
#VERSION=`echo $VERSION | cut -d \= -f2`
#PACKAGENAME=director-$VERSION-setup.bin

PACKAGENAME=Cloud-Director-2.0.0-setup.bin
ARCHIVEFILE=director.tar.gz
DIRECTORY=./linux_x86_64
. Expiration
cd  program
../shc -e $DAY/$MONTH/$YEAR -r -f _daemon
../shc -e $DAY/$MONTH/$YEAR -r -f director 
cp -f _daemon.x ../bin/_daemon
cp -f director.x ../bin/director
gcc -o vrsync -DYEAR=$YEAR -DMONTH=$MONTH -DDAY=$DAY ntptime.c
cp -f vrsync ../bin/vrsync
cd ..
Exp_time=`date -d "$YEAR-$MONTH-$DAY 00:00:00" +"%s"`
sed -i "/Exp_time=/cExp_time=$Exp_time" install.sh
./shc -e $DAY/$MONTH/$YEAR -r -f install.sh 
mv install.sh.x install.bin
tar zcf $ARCHIVEFILE install.bin bin ../vrsmsz/django-master ../vrsmsz/nginx ../vrsmsz/uploadmodule $DIRECTORY/lib ../vrsmsz/Python-3.7.5 #python-3.7.7
cat pre-install.sh $ARCHIVEFILE  > $PACKAGENAME
rm -rf $ARCHIVEFILE install.bin install.sh.x.c 
chmod a+x $PACKAGENAME
