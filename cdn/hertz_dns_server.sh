cd tarball
wget http://geolite.maxmind.com/download/geoip/api/c/GeoIP-1.4.6.tar.gz
wget http://web.irridia.com/patches/bind-9.9.5-P1-geoip-1.4.patch
wget http://ftp.isc.org/isc/bind9/9.9.5-P1/bind-9.9.5-P1.tar.gz
cd ../build
tar xzf GeoIP-1.4.6.tar.gz bind-9.9.5-P1.tar.gz
cd GeoIP-1.4.6 && ./configure --prefix=/home/chenzhong/.temp/diycdn/install/geoip 
make -j24 && make install
cd ../bind-9.9.5-P1 && patch -p0 < ../../tarball/bind-9.9.5-P1-geoip-1.4.patch
export LD_LIBRARY_PATH=/home/chenzhong/.temp/diycdn/install/geoip/lib
CFLAGS="-I/home/chenzhong/.temp/diycdn/install/geoip/include" LDFLAGS="-L/home/chenzhong/.temp/diycdn/install/geoip/lib -lGeoIP" ./configure --prefix=/home/chenzhong/.temp/diycdn/install/named --with-geoip=/home/chenzhong/.temp/diycdn/install/geoip/share/GeoIP --with-openssl=no
make -j24 && make install
mkdir -v /etc/named
touch /etc/named/named.conf
# compile named.conf
# editting zone files defined in named.conf
# ./install/named/bin/dig -t NS . > named.ca
#
./install/named/sbin/named-checkconf /etc/named/named.conf
./install/named/sbin/named -4 -c /etc/named/named.conf -d 1

# confirm setup and valid 
netstat -tnlup | grep named
# set client resolv.conf include bind server
echo nameserver 192.168.61.23 > /etc/resolv 
./install/named/bin/dig -t A www.tuchao.com
