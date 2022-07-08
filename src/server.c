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
#include"universal.h"


int Accept(int listenfd, struct sockaddr_in* cliaddr, socklen_t* clilen);

/*Thread work*/
void* process_request(void* connfd);
size_t read_message(int connectionfd, char* rcvbuf, size_t buflen);
size_t readmsg(int connectionfd, char* rcvbuf, size_t buflen);
int parsemsg_http(char* rcvbuf, size_t buflen, HTTP_req* request);
int respond(int connectionfd, const char* rcvbuf, size_t buflen);

int echo_messages(int connectionfd);

enum REQ_TYPES {
	HTTP,
	LINK,
	DLOAD
};

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

	char rcvbuf[MAXLEN];
	size_t buflen;
	const char* min_http_res = "\nHTTP/1.1 200 OK\nContent-Length: 13\nContent-Type: text/plain; charset=utf-8\n\nHello World!";
	const char* http_head_ok = "\nHTTP/1.1 200 OK\nContent-Length: 150\nContent-Type: text/plain; charset=utf-8\n\n";

	/*http*/
	/*int prc;*/
	/*const char* method;*/
	/*size_t methodlen;*/
	/*const char* path;*/
	/*size_t pathlen;*/
	/*struct phr_header headers[100];*/
	/*size_t num_headers;*/
	/*int minor_version;*/

	while(1){
		buflen = readmsg(connectionfd, rcvbuf, MAXLEN);
		if(buflen == 0){
			printf("Client sent nothing, disconnecting\n");	
			close(connectionfd);
			pthread_exit(0);
		}
		
		int prc;
		HTTP_req htrq;
		prc = parsemsg_http(rcvbuf, buflen, &htrq);
		if(prc > 0){
			printf("HTTP request received:   %s : %d\n", cliaddr_formatted, cliaddr.sin_port);
			printf("method: %.*s\n", (int) htrq.methodlen, htrq.method);
			printf("path: %.*s\n", (int) htrq.pathlen, htrq.path);
			printf("\n");

			char response[8192];
			build_http_response(&htrq, response, 8192);

		}



		//Create seperate buffer lengths
		char filepath[MAXLEN];
		/*strncpy(filepath, path, pathlen );*/

		FILE* sendfp;
		sendfp = fopen("gsr.html", "r");
		struct stat send_stats;
		fseek(sendfp, 0, SEEK_END);
		size_t filelen = ftell(sendfp);
		char* filebuf = malloc(filelen + 1);
		int frc;
		fseek(sendfp, 0, SEEK_SET);
		frc = fread(filebuf, filelen, 1, sendfp); //read one filelen sized chunk
		fclose(sendfp);
		printf("file read\n");


		size_t headerlen = strlen(http_head_ok);
		char* response = malloc(headerlen + filelen + 1);
		strncpy(response, http_head_ok, headerlen);
		strncpy(response + headerlen, filebuf, filelen + 1);
		/*response[content_length_index]*/
		char conlenstr[4];
		snprintf(conlenstr, 4, "%ld", headerlen + filelen + 1);
		strncpy(&response[content_length_index], conlenstr, 3);

		printf("%s\n", conlenstr);

		respond(connectionfd, response, strlen(response));
		free(filebuf);
		free(response);
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


int parsemsg_http(char* buffer, size_t buflen, HTTP_req* rq){
	/*parse request*/
	size_t numhead = sizeof(rq->headers) / sizeof(rq->headers[0]);
	/*prc */
	int prc;
	prc = phr_parse_request(buffer, buflen, &rq->method, &rq->methodlen, 
			&rq->path, &rq->pathlen, &rq->minor_version, rq->headers, &numhead, 0);

	if(prc < 0){
		printf("HTTP parse error: %d\n", prc);
		pthread_exit(0);
	}
	return prc;
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


