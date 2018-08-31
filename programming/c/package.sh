#!/bin/bash
#VERSION=`grep VERSION install.sh | head -1`
#VERSION=`echo $VERSION | cut -d \= -f2`
#PACKAGENAME=smsx-$VERSION-setup
#ARCHIVEFILE=smsx.tar.gz
MODULES_PATH=
check_root()
{
    if [ "root" != "$USER" ]; then
        echo "Permission denied! Please use root try again."
        exit 0;
    fi
}
check_root;
basepath=$(cd `dirname $0`; pwd)
ngx_project_path=/home/nginx
ngx_version=1.14.0
ngx_root_path=$ngx_project_path/nginx-$ngx_version
ngx_prefix="/home/vlc-project/vlc-3.0.0-git/contrib/native/itv/cdn.test/build_package_c/tmp/nginx-build"
ngx_config_path="conf/ngx.conf"
ngx_exec="itvginx"
itv_ffmpeg="itvenc"
cd $ngx_root_path
#./nginx_configure.sh $ngx_project_path $ngx_prefix $ngx_config_path
make 
echo "cp objs/nginx $basepath/org"
cp objs/nginx $basepath/org/$ngx_exec
cd - > dev/null

#rm -f $PACKAGENAME
export PKG_CONFIG_PATH=/home/cerbero-1.14/build/dist/linux_x86_64/lib/pkgconfig:/usr/local/lib/pkgconfig
gcc `pkg-config --libs  --cflags glib-2.0 libgtop-2.0` proxy.c `pkg-config --libs  --cflags glib-2.0 libgtop-2.0` \
    -DITV_GINX_OFFSET=34765425 -DITV_ENC_OFFSET=34765425 \
    -DNGX_PREFIX=\"$ngx_prefix\" -DNGX_EXEC_NAME=\"$ngx_exec\" -DNGX_CONFIG_PATH=\"$ngx_config_path\" -DNGX_MIME_PATH=\"conf/mime.conf\" \
    -DENC_EXEC_NAME=\"$itv_ffmpeg\" \
    -o proxy_temp
let itvginx_length=`ls -l org/$ngx_exec | grep -oE "[0-9]+" | sed -n '2p'`
let itvginx_offset=`ls -l proxy_temp | grep -oE "[0-9]+" | sed -n '2p'`
let itvenc_offset=itvginx_offset+itvginx_length
gcc `pkg-config --libs  --cflags glib-2.0 libgtop-2.0` proxy.c `pkg-config --libs  --cflags glib-2.0 libgtop-2.0` \
	-DITV_GINX_OFFSET=$itvginx_offset -DITV_ENC_OFFSET=$itvenc_offset \
    -DNGX_PREFIX=\"$ngx_prefix\" -DNGX_EXEC_NAME=\"$ngx_exec\" -DNGX_CONFIG_PATH=\"$ngx_config_path\" -DNGX_MIME_PATH=\"conf/mime.conf\" \
    -DENC_EXEC_NAME=\"$itv_ffmpeg\" \
	 -o org/proxy
cat org/{proxy,$ngx_exec,$itv_ffmpeg} > bin/proxy
chmod 777 bin/proxy
rm -f proxy_temp
#tar zcf $ARCHIVEFILE bin html logs modules 
#cat install.sh $ARCHIVEFILE  > $PACKAGENAME
#rm $ARCHIVEFILE
#chmod a+x $PACKAGENAME
