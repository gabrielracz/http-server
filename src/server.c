#include <sys/types.h>
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

#include "http.h"
#include "content.h"
#include "logger.h"

#define LOGGING 1

/*Thread work*/
static void* process_request(void* connfd);

static volatile unsigned long global_bytes_count;
pthread_mutex_t bytes_lock;

int server_on(int port){
	int connectionfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in 	cliaddr, servaddr;

    pthread_mutexattr_t bytes_attr;
    pthread_mutex_init(&bytes_lock, &bytes_attr);

	/*bzero(&servaddr, sizeof(servaddr));*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);


	/* Prepare the socket */
	int listenfd;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1){
		log_perror("socket");
		return 1;
    }
	

	int rc;
	while ((rc = bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)))){
		log_perror("bind");
		sleep(1);
	}

	rc = listen(listenfd, 15);
	if(rc ==-1) {
		log_perror("listen");
		return 3;
	}

	//try to stop server from waiting on port
	int en = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&en, sizeof(int));
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&en, sizeof(int));

	struct linger lin;
	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(int));

	struct timeval tv;
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	int num_workers = 10;
	int dispatched = 0;
	int request_count = 0;
	pthread_t* thread_pool[num_workers];
	for(int i = 0; i < num_workers; i++){
		thread_pool[i] = malloc(sizeof(pthread_t));
	}
	int thread_index = 0;


    content_init();

	int server_on = 1;
	log_info("server on...");
	log_break();
	while(server_on){
		int* cfd_ptr;
		cfd_ptr = malloc(sizeof(cfd_ptr));
		if(cfd_ptr == NULL) {
			log_perror("cfd malloc");
			return 4;
		}

		clilen = sizeof(cliaddr);
		connectionfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
		if(connectionfd < 0){
			if(errno == EINTR || errno == EAGAIN){
				free(cfd_ptr);
				continue;
			}else{
				log_perror("accept");
			}
		}
		*cfd_ptr = connectionfd;

		/*Dispatch the thread*/
		int i = thread_index++ % num_workers;
		request_count++;
		pthread_t* next_thread = thread_pool[i];
		pthread_create(next_thread,  NULL, &process_request, (void*) cfd_ptr);
	}
	return 0;
}

/*Thread Main*/
static void* process_request(void* connfd) {
	/*thread init*/
	int connectionfd = *((int*)connfd);
	free(connfd);
	pthread_detach(pthread_self());

	/*Get client address*/
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	getpeername(connectionfd, (struct sockaddr*) &cliaddr, &clilen);
	char cliaddr_str[INET_ADDRSTRLEN];
	if(inet_ntop(AF_INET,(void*) &cliaddr.sin_addr, cliaddr_str, INET_ADDRSTRLEN)){
		/*printf("connected: %s:%d\n", cliaddr_str, cliaddr.sin_port);*/
	}

    char* r = calloc(1, 16384);
    Buffer rcv_buffer = {.ptr=r, .len=0, .size=16384};

    HttpRequest* rq = http_create_request(rcv_buffer, cliaddr_str);
    HttpResponse* res = http_create_response();

	int i = 0; 
    while(rq->parse_status != PARSE_COMPLETE) {
        int bytes_received =  recv(connectionfd, rcv_buffer.ptr + rcv_buffer.len, rcv_buffer.size - rcv_buffer.len, 0);
        if(bytes_received == 0){ goto connection_exit; } //client disconnect
        rcv_buffer.len += bytes_received;
        http_update_request_buffer(rq, rcv_buffer);
        http_parse(rq, res);
    }

    size_t bytes_sent = 0;
    while(rq->status != REQUEST_COMPLETE) {
        http_handle_request(rq, res);

        size_t b = sendmsg(connectionfd, &res->msg, MSG_NOSIGNAL);
		if(b == -1) {
			log_perror("sendmsg");
			goto connection_exit;
		}
        bytes_sent += b;
    }

    log_info("%-7.*s%-40.*s%.3s %-10zu - %s", 
             rq->method_str.len, rq->method_str.ptr, rq->path.len, rq->path.ptr, 
             http_status_code(res), bytes_sent, rq->addr);

connection_exit:
	http_destroy_request(rq);
    http_destroy_response(res);
	free(rcv_buffer.ptr);
	close(connectionfd);
    pthread_exit(0);
	return 0;
}
