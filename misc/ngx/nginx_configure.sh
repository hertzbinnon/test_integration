#!/bin/sh

echo "configure start ..."
 ./configure \
--prefix=/home/nginx/build \
--add-module=../nginx_mod_h264_streaming-2.2.7 \
--with-pcre=../pcre-8.33 \
--with-zlib=../zlib-1.2.8 \
--with-http_dav_module \
--with-http_flv_module \
--with-http_stub_status_module \
--without-http_scgi_module \
--without-http_uwsgi_module \
--without-http_gzip_module \
--without-http_ssi_module \
--without-http_memcached_module \
--without-http_empty_gif_module \
--without-mail_pop3_module \
--without-mail_imap_module \
--without-mail_smtp_module \
--with-http_ssl_module \
--with-stream_ssl_module \
--add-module=../nginx-rtmp-module \
--with-cc-opt=-I/home/cerbero-1.14/build/dist/linux_x86_64/include \
--with-ld-opt=`-L/home/cerbero-1.14/build/dist/linux_x86_64/lib -Wl, -rpath=/home/cerbero-1.14/build/dist/linux_x86_64/lib` \
--with-openssl=/home/cerbero-1.14/build/sources/linux_x86_64/openssl-1.1.0g/ \
#--with-debug \
#--without-http_proxy_module
 echo "configure end!"
