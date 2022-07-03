#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include<strings.h>
#include<sys/socket.h>
#include<sys/types.h>  //pid_t
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY
#include<memory.h>
#include<unistd.h>
#include"defines.h"

int Accept(int listenfd, struct sockaddr_in* cliaddr, socklen_t* clilen);
int echo_messages(int connectionfd);
int read_message(int connectionfd, char* buffer, size_t buflen);

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
	char store[1024];
	for( ; ; ) {
		connectionfd = Accept(listenfd, &cliaddr, &clilen);
		childpid = fork();
		if(childpid == 0){
			close(listenfd);
			read_message(connectionfd, store, 1024);
			echo_messages(connectionfd);
			close(connectionfd);
			exit(0);
		}
	}

	return 0;
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

int read_message(int connectionfd, char* bufer, size_t buflen){
	printf("%ld %ld %ld", sizeof(size_t), sizeof(pid_t),sizeof(long long));
}

int echo_messages(int connectionfd){
	unsigned char OK = ACK;
	char store[256];
	int bytes_read;
	while(1) {
		bzero(store, 256);
		printf("[] ");
		bytes_read = recv(connectionfd, store, 256, 0);
		if(bytes_read == 0){
			printf("Client sent nothing, disconnecting\n");
			return 0;
		}
		printf("%s", store);
		send(connectionfd, &OK, sizeof(OK), 0);
	}

}


