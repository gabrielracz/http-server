#include <arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<memory.h>
#include<unistd.h>

#include<pthread.h>

#include<sys/socket.h>
#include<sys/types.h>  //pid_t
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY

#include"defines.h"


int Accept(int listenfd, struct sockaddr_in* cliaddr, socklen_t* clilen);

/*Thread work*/
void* process_request(void* connfd);
int read_message(int connectionfd, char* buffer, size_t buflen);
int respond(int connectionfd, char* buffer, size_t buflen);

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


	while(server_on){
		int* cfd_ptr;
		cfd_ptr = malloc(sizeof(cfd_ptr));
		if(cfd_ptr == NULL) {err("malloc error in serving loop", EXIT);}

		*cfd_ptr = Accept(listenfd, &cliaddr, &clilen);

		/*Dispatch the thread*/
		int i = thread_index % workers;
		thread_index++;
		pthread_t* next_thread = thread_pool[i];
		pthread_create(next_thread,  NULL, &process_request, (void*) cfd_ptr);
	}

	return 0;
}

void* process_request(void* connfd){
	int connectionfd = *((int*)connfd);
	free(connfd);

	pthread_detach(pthread_self());
	char buffer[MAXLEN];
	size_t buflen;
	
	while(1){
		/*Message msg;*/
		buflen = read_message(connectionfd, buffer, MAXLEN);
		if(buflen == 0){
			printf("Client sent nothing, disconnecting\n");	
			close(connectionfd);
			pthread_exit(NULL);
		}
		struct sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		getpeername(connectionfd, (struct sockaddr*) &cliaddr, &clilen);
		char address[MAXLEN];
		if(inet_ntop(AF_INET,(void*) &cliaddr.sin_addr, address, MAXLEN));
			printf("REQ: %s:%d\n", address, cliaddr.sin_port);
		respond(connectionfd, buffer, buflen);
	}

	close(connectionfd);
	return 0;
}

int respond(int connectionfd, char* buffer, size_t buflen) {
	/*char response[] = "OK";*/

	return send(connectionfd, buffer, buflen, 0);

}

int Accept(int listenfd, struct sockaddr_in* cliaddr, socklen_t* clilen){
	printf("Waiting for client...\n");
	*clilen = sizeof(*cliaddr);
	int connfd;
	connfd = accept(listenfd, (struct sockaddr*) cliaddr, clilen);
	if(connfd < 0)
		err("Connection error", EXIT);

	return connfd;
}

int read_message(int connectionfd, char* buffer, size_t buflen){
	int bytes_read;
	bzero(buffer, buflen);
	bytes_read = recv(connectionfd, buffer, buflen, 0);
	return bytes_read;
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


