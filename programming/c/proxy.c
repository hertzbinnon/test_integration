#include <glib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <glibtop/cpu.h>

//#ifndef ITV_ENC_OFFSET
//program()
//#endif
int getLocalInfo(const gchar* filename)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = {0};
    char ip[32] = {0};
    char broadAddr[32] = {0};
    char subnetMask[32] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)){
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        //printf("interface num = %d\n", interfaceNum);
		gchar cmd[512],tmp[1024];
        while (interfaceNum-- > 0){
            //printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);
			memset(cmd,0,512);
			sprintf(cmd,"ethtool %s | grep Speed | grep -oE '[0-9.]+'",buf[interfaceNum].ifr_name);
			sprintf(tmp,"%s >> %s",cmd,filename);
			system(tmp);

            //ignore the interface that not up or not runing  
            ifrcopy = buf[interfaceNum];
            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy)){
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);

                close(fd);
                return -1;
            }

            //get the mac of this interface  
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum]))){
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],

                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                //printf("device mac: %s\n", mac);
            }else{
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface  

            if (!ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum])){
                snprintf(ip, sizeof(ip), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));
                //printf("device ip: %s\n", ip);
            }else{
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface  

            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum])){
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_broadaddr))->sin_addr));
                //printf("device broadAddr: %s\n", broadAddr);
            }else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }
  
            //get the subnet mask of this interface  
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum])){
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_netmask))->sin_addr));
                //printf("device subnetMask: %s\n", subnetMask);
            }
            else{
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;

            }
        }
    }else{
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }
  
    close(fd);

    return 0;
}
void get_os_version(void){
	gchar* filename="/etc/redhat-release";
	gchar buf[512]="",tmp[512]="";
	int f=0,max=0,min=0,tag=0;
	gchar dis[16],ken[16],ver[16];
	guint32 this_pid=getpid();
	int cpus,mem,eth0,eth1,eth2,eth3;
	float hz;

	if((f = g_open(filename,O_RDONLY)) < 0){
		g_error("%s",g_strerror(errno));
	}
	if(read(f,(void*)buf,sizeof(buf)) <0){
		g_error("%s",g_strerror(errno));
	}
	buf[strlen(buf)-1]=0;
	sscanf(buf,"%s %s %s %d.%d.%d",dis,ken,ver,&max,&min,&tag);
	//g_printf("%d.%d.%d \n",max,min,tag);
	close(f);

	//sprintf(tmp,"/proc/%d/fd/0",this_pid);
	sprintf(tmp,"/tmp/cmutils",this_pid);
	gchar shell_cmd_cpus[]="cat /proc/cpuinfo | grep 'physical id' | uniq | wc -l";
	memset(buf,0,512);
	sprintf(buf,"%s > %s",shell_cmd_cpus,tmp);
	system(buf);
	//read(0,buf,2);
	//g_printf("CPUs %s\n",buf);

	memset(buf,0,512);
	gchar shell_cmd_hz[]="cat /proc/cpuinfo | grep 'model name' | uniq | cut -d @ -f2 | grep -oE '[0-9.]+'";
	sprintf(buf,"%s >> %s",shell_cmd_hz,tmp);
	system(buf);
	//read(0,buf,3);
	//g_printf("CPUhz%s\n",buf);

	memset(buf,0,512);
	gchar shell_cmd_mem[]="cat /proc/meminfo | grep 'MemTotal' | uniq | cut -d @ -f2 | grep -oE '[0-9.]+'";
	sprintf(buf,"%s >> %s",shell_cmd_mem,tmp);
	system(buf);
	//read(0,buf,3);
	//g_printf("mem%s\n",buf);


	getLocalInfo(tmp);

	memset(buf,0,512);
	if((f = g_open(tmp,O_RDONLY)) < 0){
		g_error("%s",g_strerror(errno));
	}
	if(read(f,(void*)buf,sizeof(buf)) <0){
		g_error("%s",g_strerror(errno));
	}

	sscanf(buf,"%d\n%f\n%d\n%d\n%d\n%d\n%d",&cpus,&hz,&mem,&eth0,&eth1,&eth2,&eth3);
	g_message("%d %f %d %d %d %d %d",cpus,hz,mem,eth0,eth1,eth2,eth3);
	if(strcmp(dis,"CentOS")){
		g_error("OS must be CentOS");
	}else if(strcmp(ken,"Linux")){
		g_error("Kernel must be Linux");
	}else if(strcmp(ver,"release")){
		g_error("release error");
	}else if(max != 7 && min != 4){ // 7.4
		g_error("Version must be 7.4");
	}else if(cpus < 32){
		g_error("The numbers of cpu core is greater than or equal to 32");
	}else if(hz < 2.9){ //3.2
		g_error("CPU Clock Speed is greater then 3.2Hz");
	}else if(mem < 32732988){
		g_error("The totals memory is greater than or equal to 32GB");
	}else if(eth0<1000 && eth1<1000&& eth2<1000&& eth3<1000){
		g_error("The speed of interface card is not less than 1000Mb/s");
	}
}

void system_check(void){
	get_os_version();
}

char nginx_conf_format[]=
"user root;\n"
"daemon off;\n"
"worker_processes  1;\n"
"error_log  logs/error.log debug;\n"
"#error_log  logs/error.log  notice;\n"
"#error_log  logs/error.log  info;\n"
"pid        logs/nginx.pid;\n"
"events {\n"
"    worker_connections  1024;\n"
"}\n"
"rtmp {\n"
"    server {\n"
"        listen %d;\n"
"        chunk_size 4000;\n"
"		#max_queue 1M;\n"
"	    max_message 4M;\n"
" \n"
"       application hls {\n"
"            live on;\n"
"            hls on;\n"
"            hls_path /tmp/hls;\n"
"        }\n"
" \n"
"	application big {\n"
"         live on;\n"
"	 	 exec %s -i rtmp://localhost:%d/big/test -c:v libx264 -x264-params key-int=25:vbv-maxrate=40000:vbv-bufsize=20000:tune=zerolatency:keyint=25:min-keyint=25:scenecut=0 -b:v 40M -minrate 40M  -maxrate 40M -acodec copy  -f flv rtmp://localhost:%d/small/test;\n"
 "       }\n"
"	application small {  \n"
 "   	live on;  \n"
"        hls on;\n"
"        hls_path /tmp/small;\n"
"		#hls_path 	/data;\n"
"		hls_fragment 5s;\n"
"		hls_playlist_length 30s;\n"
"		#hls_base_url http://192.168.61.22:58082/small;\n"
"		}\n"
 "   }\n"
"}\n"
" \n"
"http {\n"
"    include       /home/smsx/etc/mime.types;\n"
"    default_type  application/octet-stream;\n"
"    sendfile        on;\n"
"    #tcp_nopush     on;\n"
"    keepalive_timeout  65;\n"
"    server {\n"
"		 listen       %d;\n"
"        server_name  %s;\n"
"        location / {\n"
"            root   html;\n"
"            index  index.html index.htm;\n"
"        }\n"
"        location /hls {\n"
"            root   /tmp;\n"
"        }\n"
"        location /small {\n"
"            root   /tmp;\n"
"        }\n"
"    }\n"
"}\n";

char nginx_mime[]=
"types {\n"
"   text/html                             html htm shtml;\n"
"   text/css                              css;\n"
"   text/xml                              xml;\n"
"   image/gif                             gif;\n"
"   image/jpeg                            jpeg jpg;\n"
"   application/x-javascript              js;\n"
"   application/atom+xml                  atom;\n"
"   application/rss+xml                   rss;\n"
" \n"
"    text/mathml                           mml;\n"
"    text/plain                            txt;\n"
"    text/vnd.sun.j2me.app-descriptor      jad;\n"
"    text/vnd.wap.wml                      wml;\n"
"    text/x-component                      htc;\n"
" \n" 
"    image/png                             png;\n"
"    image/tiff                            tif tiff;\n"
"    image/vnd.wap.wbmp                    wbmp;\n"
"    image/x-icon                          ico;\n"
"    image/x-jng                           jng;\n"
"    image/x-ms-bmp                        bmp;\n"
"    image/svg+xml                         svg svgz;\n"
"    image/webp                            webp;\n"
" \n"
"    application/java-archive              jar war ear;\n"
"    application/mac-binhex40              hqx;\n"
"    application/msword                    doc;\n"
"   application/pdf                       pdf;\n"
"   application/postscript                ps eps ai;\n"
"   application/rtf                       rtf;\n"
"   application/vnd.ms-excel              xls;\n"
"   application/vnd.ms-powerpoint         ppt;\n"
"   application/vnd.wap.wmlc              wmlc;\n"
"   application/vnd.google-earth.kml+xml  kml;\n"
"   application/vnd.google-earth.kmz      kmz;\n"
"   application/x-7z-compressed           7z;\n"
"   application/x-cocoa                   cco;\n"
"   application/x-java-archive-diff       jardiff;\n"
"   application/x-java-jnlp-file          jnlp;\n"
"   application/x-makeself                run;\n"
"   application/x-perl                    pl pm;\n"
"   application/x-pilot                   prc pdb;\n"
"   application/x-rar-compressed          rar;\n"
"   application/x-redhat-package-manager  rpm;\n"
"   application/x-sea                     sea;\n"
"   application/x-shockwave-flash         swf;\n"
"   application/x-stuffit                 sit;\n"
"   application/x-tcl                     tcl tk;\n"
"   application/x-x509-ca-cert            der pem crt;\n"
"   application/x-xpinstall               xpi;\n"
"   application/xhtml+xml                 xhtml;\n"
"   application/zip                       zip;\n"
" \n"
"   application/octet-stream              bin exe dll;\n"
"   application/octet-stream              deb;\n"
"   application/octet-stream              dmg;\n"
"   application/octet-stream              eot;\n"
"   application/octet-stream              iso img;\n"
"   application/octet-stream              msi msp msm;\n"
" \n"
"    audio/midi                            mid midi kar;\n"
"    audio/mpeg                            mp3;\n"
"    audio/ogg                             ogg;\n"
"    audio/x-m4a                           m4a;\n"
"    audio/x-realaudio                     ra;\n"
" \n"
"   video/3gpp                            3gpp 3gp;\n"
"   video/mp4                             mp4;\n"
"   video/mpeg                            mpeg mpg;\n"
"   video/quicktime                       mov;\n"
"   video/webm                            webm;\n"
"   video/x-flv                           flv;\n"
"   video/x-m4v                           m4v;\n"
"   video/x-mng                           mng;\n"
"   video/x-ms-asf                        asx asf;\n"
"   video/x-ms-wmv                        wmv;\n"
"   video/x-msvideo                       avi;\n"
"}\n";

static GPid pid=-1; //itvginx
static gchar* temp_bin_pathname=NGX_PREFIX;
static gchar* nginx_name=NGX_PREFIX"/"NGX_EXEC_NAME;
static gchar* nginx_conf_name=NGX_PREFIX"/"NGX_CONFIG_PATH;
static gchar* nginx_conf_mime=NGX_PREFIX"/"NGX_MIME_PATH;
static gchar* enc_name = NGX_PREFIX"/"ENC_EXEC_NAME;
static int exit_status=0;

static gboolean 
setup_itvginx(void){
	GError* err=NULL;
	gchar command_line[1024]="";
	sprintf(command_line,"%s -p %s -c %s",nginx_name,temp_bin_pathname,nginx_conf_name);
	//g_printf("%s\n",command_line);
	//g_spawn_command_line_async(command_line,&err);
	gboolean retval;
	gchar **argvs = NULL;

	g_return_val_if_fail (command_line != NULL, FALSE);
	if (!g_shell_parse_argv (command_line,NULL, &argvs,&err))
		return FALSE;
	retval = g_spawn_async (NULL,argvs,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&pid,&err);
	if(!retval) 
		return FALSE;
	g_strfreev (argvs);
	//g_print("child %d \n", pid);
	
	return TRUE;
}

static gboolean
on_sig_timeout (gpointer data)
{
  GMainLoop *loop = data;
  gchar str_pid[16]="";
  sprintf(str_pid,"/proc/%d",pid);
  //g_printf("watch server\n");
  if(exit_status==1)
  	return TRUE;
  if( access(str_pid, F_OK|R_OK) !=0){
  		g_printf("itvenc pid = %s is exited,ready to restart \n",str_pid);
		setup_itvginx();	
  }
  g_timeout_add(1000,on_sig_timeout,loop);
  return TRUE;
}

static gboolean on_sig_received (gpointer user_data)
{
  GMainLoop *loop = user_data;
  exit_status=1;
  g_printf("Recived exit signal \n");
  kill(pid,SIGINT);
  g_main_loop_quit (loop);
  //sig_received = TRUE;
  //sig_counter ++;
  return TRUE;
}

static void signal_handler(GMainLoop *mainloop){
	g_unix_signal_add (SIGTERM, on_sig_received, mainloop);
	//g_unix_signal_add (SIGALRM, on_sig_received, mainloop);
	g_unix_signal_add (SIGINT , on_sig_received, mainloop);
	//g_unix_signal_add (SIGKILL, on_sig_received, mainloop);
}

int main(int argc, char** argv){
	int itv_nginx_size=ITV_GINX_OFFSET;
	int itv_enc_size=ITV_ENC_OFFSET;
	int rtmp_port=1935;
	int http_port=80;
	char host[] = "192.168.61.22";
	// first get system info 
	//get_system_info();
	// check if system condition is fit to run proxy
	//check_system_info();
	// auth
	//auth();
	// 
	system_check();
	int src_fd=-1,dst_fd=-1;
	int fd = -1;
	if( access(temp_bin_pathname, F_OK|R_OK|W_OK) ==0){
		char command_line[]="";
		sprintf(command_line,"rm -rf %s\0",temp_bin_pathname);
		system(command_line);
		//exit(0);
	}
		if(mkdir(temp_bin_pathname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) !=0){
			printf("working path crate failed\n");
			goto error;
		}
		char logs_name[]="/usr/share/lanTing/logs";
		if(mkdir(logs_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) !=0){
			printf("working path crate failed\n");
			goto error;
		}
	
	char exec_name[512];
	int count;
	count = readlink( "/proc/self/exe",exec_name,512);
	if (count < 0 || count >= 512)
	{perror("??:");}
	exec_name[count] = '\0';
 
	if((src_fd = open(exec_name,O_RDONLY)) < 0){
		perror("??");
		goto error;
	}
	if((dst_fd = open("/tmp/dstination",O_RDWR|O_CREAT)) < 0){
		perror("??");
		goto error;
	}
	struct stat st;
	int size = 0;
	char* ptr = NULL;
	char c[1024*4]="";
	memset(c,0,1024*4);

	size = fstat(src_fd,&st);
	if(size < 0){
		printf("error\n");
		close(src_fd);
		goto error;
	}
	size = st.st_size;
	//printf("%s %d\n",exec_name,size);

	while( read(src_fd,c,sizeof(c)) >0 ){
		write(dst_fd,c,sizeof(c));
	}
	close(src_fd);
	ptr = (char*)mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,dst_fd,0);
	if( ptr == NULL || ptr==(void*)-1 ) { perror("mmap");close(dst_fd);goto error;}
	//printf("%c %c %c %c %c \n",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4]);
	// 
	if( (fd = open(nginx_name,O_RDWR|O_CREAT,S_IXUSR)) < 0){
		perror("open server failed");
		goto error;
	}else{
		//g_printf("%c %c %c %c %c \n",(ptr+itv_nginx_size)[0],(ptr+itv_nginx_size)[1],(ptr+itv_nginx_size)[2],(ptr+itv_nginx_size)[3],(ptr+itv_nginx_size)[4]);
		if(write(fd,ptr+itv_nginx_size,size-itv_nginx_size) != size-itv_nginx_size ){
			perror("??:");
			goto error;
		}
		//printf("write error %d %d %d\n",size,itv_nginx_size,size-itv_nginx_size);
		close(fd);
	}
	if( (fd = open(enc_name,O_RDWR|O_CREAT,S_IXUSR)) < 0){
		printf("open rtmp failed\n");
		goto error;
	}else{
		write(fd,ptr+itv_enc_size,size-itv_enc_size);
		close(fd);
	}
	munmap(ptr,size);
	close(dst_fd);

	gchar nginx_conf[10*1024]="";
	memset(nginx_conf,0,10*1024);
	fd = -1;
	if( (fd = open(nginx_conf_name,O_RDWR|O_CREAT)) < 0){
		g_printf("open server conf failed\n");
		goto error;
	}else{
		sprintf(nginx_conf,nginx_conf_format,rtmp_port,enc_name,rtmp_port,rtmp_port,http_port,host);
		//g_printf("%s\n",nginx_conf);
		write(fd,nginx_conf,strlen(nginx_conf));
		close(fd);
	}
	fd=-1;
	if( (fd = open(nginx_conf_mime,O_RDWR|O_CREAT)) < 0){
		g_printf("open server conf failed\n");
		goto error;
	}else{
		write(fd,nginx_mime,strlen(nginx_mime));
		close(fd);
	}
	// process commline line params
	// if itvginx 
	GMainLoop *loop;
	loop = g_main_loop_new (NULL, FALSE);
	signal_handler(loop);
	g_timeout_add(1000,on_sig_timeout,loop);
	if(!setup_itvginx()){
		g_printf("rtmp setup failed\n");
		goto error;
	}
	g_main_loop_run (loop);
	g_printf("exit\n");
	return ;
error:
	printf("error\n");
	return;
}
