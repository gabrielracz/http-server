#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<memory.h>
#include<unistd.h>
#include<errno.h>

#include<pthread.h>

#include<sys/socket.h>
#include<sys/types.h>  //pid_t
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY

#include "../phttp/picohttpparser.h"

#include"defines.h"


int Accept(int listenfd, struct sockaddr_in* cliaddr, socklen_t* clilen);

/*Thread work*/
void* process_request(void* connfd);
size_t read_message(int connectionfd, char* buffer, size_t buflen);
size_t readmsg(int connectionfd, char* buffer, size_t buflen);
int respond(int connectionfd, const char* buffer, size_t buflen);

int echo_messages(int connectionfd);

int main(int argc, char** argv){
	int listenfd;
	int connectionfd;
	int rc;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in 	cliaddr, servaddr;

	/*bzero(&servaddr, sizeof(servaddr));*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = SERV_ADDR;
	servaddr.sin_port = htons(SERV_PORT);


	/* Prepare the socket */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0) err("Socket initialization", EXIT);

	rc = bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	if(rc) err("Socket bind", EXIT);

	rc = listen(listenfd, 15);
	if(rc < 0) err("Socket listen", EXIT);

	int enable = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(int));
	int en = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char *)&en, sizeof(int));

	struct linger lin;
	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, (const char *)&lin, sizeof(int));


	const char* FIN = "FIN";
	char store[MAXLEN];
	int server_on = 1;

	int workers = 10;
	int dispatched = 0;
	int requets_served = 0;
	pthread_t* thread_pool[10];
	for(int i = 0; i < 10; i++){
		thread_pool[i] = malloc(sizeof(pthread_t));
	}
	int thread_index = 0;


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
	int connectionfd = *((int*)connfd);
	free(connfd);
	pthread_detach(pthread_self());

	/*Print client address*/
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	getpeername(connectionfd, (struct sockaddr*) &cliaddr, &clilen);
	char cliaddr_formatted[MAXLEN];
	inet_ntop(AF_INET,(void*) &cliaddr.sin_addr, cliaddr_formatted, MAXLEN);
	/*if(inet_ntop(AF_INET,(void*) &cliaddr.sin_addr, cliaddr_formatted, MAXLEN))*/
		/*printf("connected: %s:%d\n", cliaddr_formatted, cliaddr.sin_port);*/

	char buffer[MAXLEN];
	size_t buflen;
	const char* min_http_res = "\nHTTP/1.1 200 OK\nContent-Length: 13\nContent-Type: text/plain; charset=utf-8\n\nHello World!";

	/*http*/
	int prc;
	char* method;
	size_t methodlen;
	char* path;
	size_t pathlen;
	struct phr_header headers[100];
	size_t num_headers;
	int minor_version;

	struct {
		char* method;
		size_t methodlen;
		char* path;
		size_t pathlen;
		struct phr_header headers[100];
		size_t num_headers;
		int minor_version;

	} HTTP;

	while(1){
		buflen = readmsg(connectionfd, buffer, MAXLEN);
		if(buflen == 0){
			printf("Client sent nothing, disconnecting\n");	
			close(connectionfd);
			pthread_exit(0);
		}
		
		/*parse request*/
		size_t numhead = sizeof(headers) / sizeof(headers[0]);
		/*prc */
		prc = phr_parse_request(buffer, buflen, &method, &methodlen, 
				&path, &pathlen, &minor_version, headers, &numhead, 0);

		if(prc < 0){
			printf("HTTP parse error: %d\n", prc);
			pthread_exit(0);
		}

		printf("HTTP request received:   %s : %d\n", cliaddr_formatted, cliaddr.sin_port);
		printf("method: %.*s\n", (int) methodlen, method);
		printf("path: %.*s\n", (int) pathlen, path);
		printf("\n");
		/*printf("path: %s\n", path);*/

		/*Process and serve requested file*/


		respond(connectionfd, min_http_res, strlen(min_http_res));
		/*printf("%s\n", buffer);*/
	}

	close(connectionfd);
	return 0;
}

int respond(int connectionfd, const char* buffer, size_t buflen) {
	/*char response[] = "OK";*/

	return send(connectionfd, buffer, buflen, 0);

}

int Accept(int listenfd, struct sockaddr_in* cliaddr, socklen_t* clilen){
	*clilen = sizeof(*cliaddr);
	int connfd;
	connfd = accept(listenfd, (struct sockaddr*) cliaddr, clilen);
	if(connfd < 0)
		err("Connection error", EXIT);

	return connfd;
}


size_t readmsg(int connectionfd, char* buffer, size_t buflen){
	return recv(connectionfd, buffer, buflen, 0);
}

/*todo: read entire message before returning*/
size_t read_message(int connectionfd, char* buffer, size_t buflen){
	int total_bytes = 0;
	int bytes_read = 0;
	int bread = 0;
	char* bufptr = buffer;
	bzero(buffer, buflen);

	while( (bytes_read = read(connectionfd, buffer + bread, buflen - bread) > 0)){
		buflen -= bytes_read;
		bufptr += bytes_read + 1;
		total_bytes += bytes_read;
		printf("%d\n", bytes_read);
	}

	if(bytes_read < 0){
		err("read error, message probably too big", EXIT);
	}


	/*bzero(buffer, MAXLEN);*/
	/*buffer[0] = 'A';*/
	/*return 1;*/

	return total_bytes;
}

int echo_messages(int connectionfd){
	unsigned char OK = ACK;
	char store[MAXLEN];
	int bytes_read;
	while(1) {
		bzero(store, MAXLEN);
		printf("[] ");
		bytes_read = recv(connectionfd, store, MAXLEN, 0);
		if(bytes_read == 0){
			printf("Client sent nothing, disconnecting\n");
			return 0;
		}
		printf("%s", store);
		send(connectionfd, &OK, sizeof(OK), 0);
	}

}


