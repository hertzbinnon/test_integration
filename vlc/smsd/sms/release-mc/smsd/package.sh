#!/bin/sh
VERSION=`grep VERSION install.sh | head -1`
VERSION=`echo $VERSION | cut -d \= -f2`
PACKAGENAME=smsd-$VERSION-setup
ARCHIVEFILE=smsd.tar.gz
MODULES_PATH=../../modules
check_root()
{
    if [ "root" != "$USER" ]; then
        echo "Permission denied! Please use root try again."
        exit 0;
    fi  
}

python_compile()
{
	python  -m py_compile $1
	chmod +x $1"c"
	mv $1"c" bin/
}
check_root;

python_compile binding.py
python_compile streaming.py
python_compile wsgi.py
python -O -m py_compile smsd.py
mv smsd.pyo bin/smsd
chown -R smsd.smsd bin/ $MODULES_PATH
rm -f $PACKAGENAME
tar zcf $ARCHIVEFILE bin $MODULES_PATH eventlet setproctitle greenlet setuptools
cat install.sh smsd.tar.gz > $PACKAGENAME 
rm $ARCHIVEFILE 
chmod a+x $PACKAGENAME 
