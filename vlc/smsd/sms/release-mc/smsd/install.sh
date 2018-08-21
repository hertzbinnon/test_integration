#! /bin/bash

#LIBTREE=smsd-3.0.0
PACKET=$LIBTREE.tar.gz
#PACKPATH=`pwd`
VERSION=2.5.2-centos7
INSTALL_PATH=/home/smsd
#
#Installing library and runtime
#
check_root()
{
    if [ "root" != "$USER" ]; then
        echo "Permission denied! Please use root try again."
        exit 0;
    fi  
}

check_root;

echo "Install ..."
basepath=$(cd `dirname $0`; pwd)

#if [ $# != 1 ] ; then
#	echo "Usage: $0 install-(absolute-)path"
#	exit 0;
#fi

#if [ -z INSTALL_PATH ]; then
#	INSTALL_PATH=/tmp/my-xxxxx-ffff
#fi

if id -u smsd >/dev/null 2>&1; then
        echo "Warning: user exists"
else
	adduser smsd 
        echo "user smsd does not exist"
fi

if [ ! -d $INSTALL_PATH ]; then 
	mkdir -p $INSTALL_PATH
fi

rm -rf $INSTALL_PATH/modules

rm -rf $INSTALL_PATH/bin

rm -rf $INSTALL_PATH/log

if [ ! -d $INSTALL_PATH ]; then 
	echo "Create install path error"
	exit 1;
fi

#if [ ! -f $PACKET ]; then 
#	echo "Packet is lost."
	#exit 1;
#fi


pushd  $INSTALL_PATH > /dev/null
    ARCHIVE=`awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $basepath/$0`
    tail -n+$ARCHIVE $basepath/$0 | tar xz
popd  > /dev/null

if [ $? != 0 ]; then
	echo "unpack error."
	exit 1;
fi

if [ ! -d $INSTALL_PATH/log ]; then 
	mkdir -p $INSTALL_PATH/log
fi

if [ ! -d /etc/itvencoder ]; then 
	mkdir -p /etc/itvencoder
fi
## bash ##
build_bash(){
	echo "#!/bin/sh"
	echo "srctree=\"$INSTALL_PATH\""
	echo
	echo "export PATH=\${srctree}/bin:\$PATH"
	echo "LD_LIBRARY_PATH=/usr/lib64:\${srctree}/modules:\$LD_LIBRARY_PATH"
	echo "export LD_LIBRARY_PATH"
	echo

	echo "VLC_PLUGIN_PATH=\${srctree}/modules"
	echo "export VLC_PLUGIN_PATH"
	echo 
#	echo "chcon -t texrel_shlib_t \${srctree}/extra/lib/*.so"
#	echo "chcon -t texrel_shlib_t \${srctree}/lib/*.so"
}

device_conf(){
	echo "# Example "
	echo "#PROXY_HOST=\"192.168.12.156 9555\""
	echo "#WEB_SERVICE_HOST=\"192.168.12.93 10000\""
	echo "#PROXY_ID=\"5222\""
	echo "HTTP_PORT=\"8091\""
	echo "DEBUG=\"1\""
}

build_runtine(){
	echo "#!/bin/sh"
	echo "version=$VERSION"
	echo "echo \"version \$version\" "
	echo "if [ \$# != 1 ] ; then
	#echo \"Usage: \$0 config-name \"
	exit 0;
fi

if [ \"\$1\" == \"reconfig\" ];then
	. /etc/smsd.conf
	wget --post-file=\"/etc/smsd_config.xml\" http://localhost:\$HTTP_PORT -O /dev/null > /dev/null 2>&1
	exit 0;
fi

if [ ! -f \$1 ]; then 
	#echo \"Input file is not exist\"
	exit 1;
fi"
	echo ""
	echo "if [ -f ./\$1 ];then
	. ./\$1 
else
	. \$1
fi"

	#echo "$1 \$PROXY_HOST \$WEB_SERVICE_HOST \$PROXY_ID"
	#echo "python $1 "
	echo "su - smsd -c \". $INSTALL_PATH/bin/bash.sh && python $1 $INSTALL_PATH \$HTTP_PORT \$DEBUG > /dev/null 2>&1 &\""
	#echo "su - smsd -c \". $INSTALL_PATH/bin/bash.sh && python $1 $INSTALL_PATH > /tmp/smsd.err 2>&1 &\""
}

build_watchdog(){
	echo "0-59 * * * * root /etc/iproxy_watch"	
}

build_init_d(){
	echo "#! /bin/bash"
	echo ". /etc/init.d/functions"
	echo "start() {" 
	echo "	echo -n \$\"Starting $1:\""
	echo "	daemon $1 /etc/$1.conf"
	#echo "	/etc/itvdaemon > /tmp/itv.log &"
	echo "	/etc/itvdaemon > $INSTALL_PATH/log/itv.log &"
	echo "	RETVAL=\$?"
	echo "	return \$RETVAL"
	echo "}"
	 
	echo "stop() {"
	#echo "	killproc $1"
	echo "	killproc itvdaemon"
	echo "	killproc smsd"
	echo "	RETVAL=\$?"
	echo "	return \$RETVAL"
	echo "}"

	echo "restart() {"
	echo "		stop"
	echo "		start"
	echo "}"
echo " 
case \"\$1\" in
  start)
  	start
	;;
  stop)
  	stop
	;;
  restart)
  	restart
	;;
  *)
	echo \$\"Usage: \$0 {start|stop|restart}\"
	exit 1
esac"

}

build_bash > $INSTALL_PATH/bin/bash.sh
device_conf > $INSTALL_PATH/bin/iproxy.conf
build_runtine $INSTALL_PATH/bin/smsd> $INSTALL_PATH/bin/iproxy
chmod +x $INSTALL_PATH/bin/iproxy

rm -f /usr/bin/smsd
#rm -f /usr/bin/iDvr
#ln -s  $INSTALL_PATH/bin/iproxy /usr/bin/iDvr
#ln -s  $INSTALL_PATH/bin/iproxy /usr/bin/smsd
mv  $INSTALL_PATH/bin/iproxy /usr/bin/smsd

#echo "Note: please config(at /etc) for smsd proxy before setup, "

#rm -f /etc/init.d/iDvr
rm -f /etc/init.d/smsd
#rm -f /etc/iDvr.conf
rm -f /etc/smsd.conf
rm -f /etc/itvencoder/itvencoder.xml
rm -f /etc/itvencoder/smsd_config.xml
rm -f /etc/cron.d/iproxy
rm -f /etc/itvdaemon

#build_init_d iDvr > /etc/init.d/iDvr
build_init_d smsd > /etc/init.d/smsd
#chmod +x /etc/init.d/iDvr
chmod +x /etc/init.d/smsd
#cp -f $INSTALL_PATH/bin/iproxy.conf /etc/iDvr.conf
mv -f $INSTALL_PATH/bin/iproxy.conf /etc/smsd.conf
mv -f $INSTALL_PATH/bin/itvencoder.xml /etc/itvencoder/itvencoder.xml.template
mv -f $INSTALL_PATH/bin/smsd_config.xml /etc/smsd_config.xml
mv $INSTALL_PATH/bin/daemon /etc/itvdaemon
chown smsd.smsd -R $INSTALL_PATH/*
cd $INSTALL_PATH
cd setuptools && python setup.py install
cd ../greenlet && python setup.py install
cd ../eventlet && python setup.py install
cd ../setproctitle && python setup.py install
cd .. && rm -rf setuptools greenlet eventlet setproctitle 
echo "Install done "
echo 64 > /proc/sys/net/ipv4/ip_default_ttl
echo "...."
exit 0;
__ARCHIVE_BELOW__
