#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
 
#define debug 1
#define TIMEOUT 3
#define JAN_1970 0x83AA7E80
//#define NTP_SERVER_1 "114.80.81.1"    //cn.pool.ntp.org
//#define NTP_SERVER_2 "122.226.192.4"  //cn.pool.ntp.org
//#define NTP_SERVER_3 "218.75.4.130"   //cn.pool.ntp.org
#define NTP_SERVER_3 "114.118.7.163"   //ntp.ntsc.ac.cn
 
struct tm exp_time={.tm_year=YEAR-1900,.tm_mon=MONTH-1,.tm_mday=DAY,.tm_hour=0,.tm_min=0,.tm_sec=0,.tm_isdst=0};

void construct_ntp_packet(char content[])
{
	long           timer;
 
	memset(content, 0, 48);
	content[0] = 0x1b; 			// LI = 0 ; VN = 3 ; Mode = 3 (client);
	
	time((time_t *)&timer);
	timer = htonl(timer + JAN_1970 );
	
	memcpy(content + 40, &timer, sizeof(timer));  //trans_timastamp
}
 
int get_ntp_time(int sockfd, struct sockaddr_in *server_addr, struct tm *net_tm) 
{
	char           content[256];
	time_t         timet;
	long           temp;
	int            addr_len = 16;
	struct timeval block_time;
	fd_set         sockfd_set;
 
	FD_ZERO(&sockfd_set);
	FD_SET(sockfd, &sockfd_set);
	block_time.tv_sec = TIMEOUT;      //time out 
	block_time.tv_usec = 0;
 
	construct_ntp_packet(content);
	if (sendto(sockfd, content, 48, 0, (struct sockaddr *)server_addr, addr_len) < 0) {
#if debug
		perror("sendto error");
#endif
		return (-1);
	}
 
	if(select(sockfd + 1, &sockfd_set, NULL, NULL, &block_time ) > 0) {
		if (recvfrom(sockfd, content, 256, 0, (struct sockaddr *)server_addr, (socklen_t *)&addr_len) < 0) {
#if debug
			perror("recvfrom error");
#endif
			return (-1);
		}
		else {
			memcpy(&temp, content + 40, 4);
			temp = (time_t)(ntohl(temp) - JAN_1970 );
			timet = (time_t)temp;
			memcpy(net_tm, gmtime(&timet), sizeof(struct tm));
 
			net_tm->tm_hour = net_tm->tm_hour + 8;  //beijing time zone
		}
	}
	else {
		return(-1);
	}
	return(0);
}
 
int main()
{
	int                  sockfd, i;
	struct tm            *net_tm;
	struct sockaddr_in   addr;
	char                 ip[4][16]= { { NTP_SERVER_3 } };
	char                 date_buf[50];
	time_t pt=0,cur=0,diff=0;
 
	net_tm = (struct tm *)malloc(sizeof(struct tm));
 
	for (i = 0 ; i < 1 ; i++ ) {
		memset(&addr, 0, sizeof(addr));
		addr.sin_addr.s_addr = inet_addr( ip[i] );
		addr.sin_port = htons(123);
		
		if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
#if debug
			perror ("socket error");
#endif
			return (-1);
		}
	
		if (get_ntp_time(sockfd, &addr, net_tm) == 0) {
			break;
		}
 
		close(sockfd);
	}
	//strftime(date_buf, sizeof(date_buf), "date -s \"%F %T\"", net_tm);
	//system(date_buf);
	cur=mktime(net_tm);
	pt = mktime(&exp_time);
	diff=pt-cur;
	//printf("==> %s\n",asctime(net_tm));
	//printf("==> %ld\n",cur);
	//printf("+==> %s\n",asctime(&exp_time));
	//printf("==> %ld\n",pt);
	printf("%ld\n",diff);
        if( diff > 0 )
		return 1;

	return (0);
}
