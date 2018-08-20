wget  http://nginx.org/download/nginx-0.8.45.tar.gz
wget http://downloads.sourceforge.net/project/varnish/varnish/2.1.2/varnish-2.1.2.tar.gz?use_mirror=cdnetworks-us-1&ts=1279434397

./install/nginx/sbin/nginx -c /home/diycdn/nginx.conf
./install/varnish/sbin/varnishd -f varnish.vcl

./install/varnish/bin/varnishstat

