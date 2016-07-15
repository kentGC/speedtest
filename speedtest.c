#include <unistd.h>  
#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#define OPTSTRING "dui:n:t:"
#define DOWNLOAD 1
#define UPLOAD	 1 << 1
#define THR_NO_DEFAULT 4
#define DEFAULT_DL_RESOURCE "/speedtest/random4000x4000.jpg"
#define DEFAULT_UL_RESOURCE "/speedtest/upload.php"
#define POST_DEFAULT_SIZE ((size_t)(1024*1024))
typedef unsigned int uint32_t ;

typedef struct speed_test_ctx speed_ctx_t;
struct speed_test_ctx {
	uint32_t host_no;
	void *(*cb)(void *);
	short thr_no;
	short action;
	char *host_name;
	char *url;

	
};
typedef struct speedtest_result speed_result_t;
struct speedtest_result {
	time_t time_down;
	time_t time_up;
	int state;
	int speed_down;
	int speed_up;
	long long totle_down;
	long long totle_up;
	
}; 

	
	  
void speedtest_help(void)
{
	printf(
			"USAGE:\n"
			"-d		enable download test     	   \n"
			"-u		enable upload test       	   \n"
			"-n [host name]	speed test server host   	   \n"
			"-t [num]	number of create pthread,default 4 \n"
			"sample:\n"
			"	speedtest -dun [url]"
			"\n"
	);
	exit(1);
	
} 

uint32_t resolve_host_by_name(const char * hostname)
{
	if (hostname == NULL) {
#if DBUG
		printf("host_name is NULL\n");
#endif	
		return 0;
	}
	struct hostent * he_ptr;
	
	he_ptr = gethostbyname(hostname);
	if (he_ptr == NULL)
	{
#if DBUG
		printf("Cannot resolve %s. h_errno is %d\n", hostname, h_errno);
#endif 
		return 0;
	}
	return *(uint32_t *)(he_ptr->h_addr);
}

uint32_t resolve_host_by_ip(const char * ip)
{
	/**/
}


	
void *download(void *arg)
{
	
	int fd = 0;
	struct sockaddr_in sin;
	speed_ctx_t *ctx = (speed_ctx_t *)arg;
	int ret;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
#if DBUG
		perror("create socket");
#endif	
		pthread_exit(NULL);
	}
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = ctx->host_no;
#if DBUG	
	printf("create socket OK and fd is %d\n",fd);
#endif

	char tmp[128] = "";
	int i = 0;
	
	char buffer[4096] = {0};
	ret = connect(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
	if (ret == -1) {
#if DBUG
		perror("connect");
#endif
		pthread_exit(NULL);
	}
#if DBUG
	printf("connect success\n");
#endif 		
	
	
	struct timeval tv_out;
	tv_out.tv_sec = 2;
	tv_out.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
SEND_REQUEST_CONTINUE:	
	snprintf(
	buffer,
    sizeof(buffer),
    "%s %s HTTP/1.1\r\n"
    "User-Agent: netspeed/0.0\r\n"
    "Accept: */*\r\n"
    "Host: %s\r\n"
    "%s%s%s"
    "\r\n",
	"GET",
    ctx->url,
    ctx->host_name,
	"",
	"",
	"");
#if DBUG
	
	printf(" request is :\n%s\n",buffer);
#endif 
	

	  
	ret = send( fd,buffer,sizeof(buffer),0);
	if (ret == -1)
    {
		if (errno == EINTR)
		{
			goto SEND_REQUEST_CONTINUE;
		} else {
#if DBUG		  
			perror("send");
#endif		  
			pthread_exit( NULL);
		}

      
    }	  
	
	int totle = 0;
	int len = 0;
	buffer[sizeof(buffer) - 1] = 0;
	int download_size = 0;
	char *ptr = NULL;
	
read_reply_header:	
	ret = recv(fd,buffer+len,sizeof(buffer) -1 - len,0);
	if ( ret == -1) {
		if (errno == EINTR) {
			goto read_reply_header;
		}
		
#if DBUG		  
		perror("recv");
#endif			
		pthread_exit( NULL);
	}
	ptr = strstr(buffer , "\r\n\r\n");
	if ( ptr == NULL ) {
		len += ret;
		if (len >= sizeof(buffer))
			pthread_exit(NULL);
		goto read_reply_header;
	} else {
#if DBUG
/*
		int count = 0;
		while (buffer[count] != 0 && buffer[count] != '\n' && buffer[count] != '\r') {
				printf("%c",buffer[count++]);
			
			
		}
		putchar('\n');
*/
		printf("response is:\n%s\n",buffer);
#endif
		len = ret - (ptr - buffer);
		ptr = strstr(buffer + totle, "Content-Length");
		if (ptr != NULL) {
			ptr += sizeof("Content-Length");
			while (*ptr == ' '||*ptr == ':') ptr++;
			memset(tmp,0,sizeof(tmp));
			i = 0;
			while (*ptr != '\n' && *ptr != '\r'&&  *ptr != ' ') {
				tmp[i] = *ptr;
				ptr++;
				i++;
				
			}
			
			
			
			download_size = atoi(tmp);
#if DBUG		  
			printf("download size is %lld\n",download_size);
#endif	
			
		}	
		
	}
	
	
	totle += len;	
	while (ret = recv(fd,buffer,sizeof(buffer) -1,0)) {
		if ( ret == -1) {
			if (errno == EINTR) {
				continue;
			}
		
#if DBUG		  
			perror("recv");
#endif			
			goto EXIT;
		} else if (totle >= download_size) 
			break;
		
		totle += ret;
		
	}	
#if DBUG
	printf("thr id : [%d] totle is %lld\n",pthread_self(),totle);
#endif 
EXIT:
	if (totle == 0)
		totle++;
	
	pthread_exit((void *)totle);	
}	
void * upload(void *arg)
{
	int fd = 0;
	struct sockaddr_in sin;
	speed_ctx_t *ctx = (speed_ctx_t *)arg;
	int ret;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
#if DBUG
		perror("create socket");
#endif	
		pthread_exit(NULL);
	}
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = ctx->host_no;
	
	char tmp[128] = "";
	int i = 0;
	
	char buffer[4096] = {0};
	
	ret = connect(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
	if (ret == -1) {
#if DBUG
		perror("connect");
#endif
		pthread_exit(NULL);
	}
#if DBUG
	printf("connect success\n");
#endif 		
	
#if DBUG	
	printf("create socket OK and fd is %d\n",fd);
#endif
SEND_REQUEST_CONTINUE:
	snprintf(
	buffer,
    sizeof(buffer),
    "%s %s HTTP/1.1\r\n"
    "User-Agent: netspeed/0.0\r\n"
    "Accept: */*\r\n"
    "Host: %s\r\n"
    "%s%lu%s"
    "\r\n",
	"POST",
    ctx->url,
    ctx->host_name,
	"Content-Length:",
	POST_DEFAULT_SIZE,
	"\r\n");
#if DBUG
	
	printf(" request is :\n%s\n",buffer);
#endif 
	struct timeval tv_out;
	tv_out.tv_sec = 2;
	tv_out.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	
	
	ret = send( fd,buffer,strlen(buffer),0);
	if (ret == -1) {
		if (errno == EINTR)
		{
			goto SEND_REQUEST_CONTINUE;
		} else {
#if DBUG		  
			perror("send");
#endif		  
			pthread_exit( NULL);
		}

      
	}
	int  totle = POST_DEFAULT_SIZE;
	int  totle_tmp = 0;
	memset(buffer,1,sizeof(buffer));
	int len = sizeof(buffer);
	while (ret = send( fd,buffer,len,0)){
		if (ret == -1)
		{
			if (errno == EINTR)
			{
				continue;
			} else {
#if DBUG		  
				perror("send");
#endif		  
				pthread_exit( (void *)totle_tmp );
			}

		} else if (totle < totle_tmp)
			break;
		
		totle_tmp += ret;
		len = totle - totle_tmp < sizeof(buffer) ? totle - totle_tmp : sizeof(buffer);
	}
	
	pthread_exit((void *)totle  );
}

int speedtest_thr(speed_ctx_t *ctx,speed_result_t *result)
{	
	if (result == NULL )
		return -1;
	
	int loop = ctx->thr_no;
	pthread_t tid[64] = {0};
	int i = 0;
	void *thr_ret = NULL;
	time_t time_var = 0;
	time_var = time(NULL);
	
#if DBUG 
	printf("thr count is %d\n",loop);
#endif	
	
	for (i = 0; i < loop;i++) {
		pthread_create(&(tid[i]),NULL,ctx->cb,(void *)ctx);

#if DBUG
		printf("create thread id is %u.\n",tid[i]);
#endif 		
		
	}
			
#if DBUG 
	printf("end of create thr ,start wait thr...\n");
#endif
	
	for (i = 0; i < loop ; i++) {
#if DBUG
		printf("wait thread id is %u.\n",tid[i]);
#endif 		
		pthread_join(tid[i],&thr_ret);
		if (thr_ret != NULL && ctx->action & DOWNLOAD) 
			result->totle_down += (int)thr_ret;
		else if (thr_ret != NULL && ctx->action & UPLOAD)
			result->totle_up += (int)thr_ret;
	}
	if (ctx->action & DOWNLOAD)
		result->time_down =   time(NULL) - time_var;
	else 
		result->time_up = time(NULL) - time_var;
	
#if DBUG 	
	printf("end of wait thr end\n");
#endif	
	
	
}

int speedtest(speed_ctx_t *ctx,speed_result_t *result)
{
	
	int var = ctx->action;
	if ( var & DOWNLOAD) {
		ctx->cb = download;
		ctx->action = var & DOWNLOAD;
		ctx->url = DEFAULT_DL_RESOURCE;
		speedtest_thr(ctx,result);
	}
		
	
		
	
	if ( var & UPLOAD) {
		ctx->cb = upload;
		ctx->action = var & UPLOAD;
		ctx->url=DEFAULT_UL_RESOURCE;
		speedtest_thr(ctx,result);
	}
		
	
	return 0;
}



int main(int argc ,char *argv[])
{
	printf("start...\n");
	speed_ctx_t *ctx = malloc(sizeof(speed_ctx_t));
	if (ctx == NULL) {
		
		
		return 0;
	}

	memset(ctx , 0, sizeof(speed_ctx_t));

	speed_result_t result;

	memset(&result , 0 , sizeof(speed_result_t  ));
	

	if (argc < 3)
		goto HELP;
	int ret = 0;
	
	while ((ret = getopt(argc, argv, OPTSTRING) )!= -1) {
#if DBUG		
		printf("ret = %c\n",ret);
#endif
		switch (ret) {
				
			case 'i':
				ctx->host_no = resolve_host_by_ip(optarg);
				break;
			case 'n':
			
				ctx->host_name = malloc(strlen(optarg)+1);
				if (ctx->host_name == NULL) {
					perror("malloc");
					goto HELP;
				}
				
				memcpy(ctx->host_name,optarg,strlen(optarg)+1);
				ctx->host_no = resolve_host_by_name(ctx->host_name);	
				
				break;
			case 't':
				ctx->thr_no = atoi(optarg);
				break;
			case 'd':
				ctx->action |= DOWNLOAD;
				break;
			case 'u':
				ctx->action |= UPLOAD;
				break;
			default :
				;
			
		}
		
	}
	
	


	
	if (ctx->host_no == 0) {

		printf("resolve host error . check your ip or hostname .\n" );

		goto HELP;
	} else { 

		ctx->action = ctx->action == 0 ?   ctx->action |= (DOWNLOAD | UPLOAD) :ctx->action;
		
		ctx->thr_no = ctx->thr_no != 0 ? ctx->thr_no : THR_NO_DEFAULT;
		
	}
	
	
#if DBUG
	printf("action is %d ,pthread num is %d ,host num is %d \n",ctx->action,ctx->thr_no,ctx->host_no);
#endif 			
	
	
	
	ret = speedtest(ctx,&result);
	if (ret = -1)
	{
		
	}
	
	int download_speed = 0;
	int upload_speed = 0;
	if (result.time_down && result.time_down)
		download_speed = result.totle_down/result.time_down;
	if (result.time_up && result.time_up)
		upload_speed = result.totle_up/result.time_up;
	
	printf("totle download :%lld b\nuse time :%d s\ndonload speed  %lld b/s\n", \
							result.totle_down,(int )result.time_down,download_speed);
	
	printf("totle upload :%lld b\nuse time :%d s\nup speed  %lld b/s\n", \
							result.totle_up,(int )result.time_up,upload_speed);
	
	
EXIT:
	if (ctx->host_name)
		free(ctx->host_name);
	if (ctx)
		free(ctx);

	
	
	
	exit(0);
HELP:
	speedtest_help();
	
}  




  
