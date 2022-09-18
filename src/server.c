#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<sys/stat.h>

#include<unistd.h>
#include<memory.h>
#include<errno.h>

#include<pthread.h>

#include<signal.h>

#include<sys/socket.h>
#include<sys/types.h>  //pid_t
#include<arpa/inet.h>  //inet_ptons
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY

#include"http.h"
#include "../phttp/picohttpparser.h"
#include "../libsha256/libsha.h"
#include"universal.h"


/*Thread work*/
void* process_request(void* connfd);
size_t readmsg(int connectionfd, char* buffer, size_t buflen);

enum REQ_TYPES {
	HTTP,
	LINK,
	DLOAD
};

int main(int argc, char** argv){
	int connectionfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in 	cliaddr, servaddr;

	/*bzero(&servaddr, sizeof(servaddr));*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = SERV_ADDR;
	//inet_aton("149.248.59.171", &servaddr.sin_addr.s_addr);
	servaddr.sin_port = htons(7120);


	/* Prepare the socket */
	int listenfd;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0) err("Socket initialization", EXIT);

	int rc;
	while ((rc = bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)))){
		printf("waiting for port...\n");
		sleep(1);
		/*if(rc) err("Socket bind", EXIT);*/
	}

	rc = listen(listenfd, 15);
	if(rc < 0) err("Socket listen", EXIT);


	//try to stop server from waiting on port
	int en;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&en, sizeof(int));
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&en, sizeof(int));

	struct linger lin;
	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(int));

	int workers = 10;
	int dispatched = 0;
	int requets_served = 0;
	pthread_t* thread_pool[10];
	for(int i = 0; i < 10; i++){
		thread_pool[i] = malloc(sizeof(pthread_t));
	}
	int thread_index = 0;


	int server_on = 1;
	printf("Server on...\n");
	while(server_on){
		int* cfd_ptr;
		cfd_ptr = malloc(sizeof(cfd_ptr));
		if(cfd_ptr == NULL) {err("malloc error in serving loop", EXIT);}

		clilen = sizeof(cliaddr);
		connectionfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
		if(connectionfd < 0){
			if(errno == EINTR)
				continue;
			else{
				err("accept error", EXIT);
			}
		}
		*cfd_ptr = connectionfd;

		/*Dispatch the thread*/
		int i = thread_index % workers;
		thread_index++;
		pthread_t* next_thread = thread_pool[i];
		pthread_create(next_thread,  NULL, &process_request, (void*) cfd_ptr);
	}
	return 0;
}

/*Thread Main*/
void* process_request(void* connfd){
	/*thread init*/
	int connectionfd = *((int*)connfd);
	free(connfd);
	pthread_detach(pthread_self());

	/*Get client address*/
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	getpeername(connectionfd, (struct sockaddr*) &cliaddr, &clilen);
	char cliaddr_str[INET_ADDRSTRLEN];
	if(inet_ntop(AF_INET,(void*) &cliaddr.sin_addr, cliaddr_str, MAXLEN)){
		/*printf("connected: %s:%d\n", cliaddr_str, cliaddr.sin_port);*/
	}

	size_t rcvlen;
	char rcvbuf[8192];
    struct static_buffer rcv_buffer;

	size_t reslen;
	size_t resbuflen = 210 * 1024 * 1024;
	//char resbuf[resbuflen];
	char* resbuf = malloc(resbuflen);
    struct static_buffer resp_buffer = {resbuf, resbuflen};

	while(1){
		rcvlen = readmsg(connectionfd, rcvbuf, MAXLEN);
		if(rcvlen == 0){
			printf("Client sent nothing, disconnecting\n");	
			break;
		}
		
		/*attempt http parse*/
		int prc;
		int htrc;
		HTTPrq rq;
		prc = http_parse(rcvbuf, rcvlen, &rq);
		if(prc > 0){
			rq.addr = cliaddr;
			strncpy(rq.addr_str, cliaddr_str, INET_ADDRSTRLEN);
			reslen = http_handle_rq(rq, resbuf, resbuflen);
			if(reslen == 0) {
				/*reslen = 0;*/
				//Handle file open errors
			}
		}else{
			memcpy(resbuf, rcvbuf, rcvlen);
			reslen = rcvlen;
		}

		size_t bsent;
		bsent = send(connectionfd, resbuf, reslen, 0);
		printf("[LOG] sent %zu\n", bsent);
	}
	free(resbuf);
	close(connectionfd);
    pthread_exit(0);
	return 0;
}

size_t readmsg(int connectionfd, char* buffer, size_t buflen){
	return recv(connectionfd, buffer, buflen, 0);
}

