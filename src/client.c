#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<memory.h>
#include <strings.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <unistd.h>
#include"defines.h"

int main(int argc, char* argv[]){
	int sockfd;
	struct sockaddr_in cliaddr, servaddr;
	int rc;

	char* localhost = "127.0.0.1";
	char* input_address;

	if(argc == 1){
		input_address = localhost;
	}else{
		input_address = argv[1];
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) err("Socket initializtion error", EXIT);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	rc = inet_pton(AF_INET, input_address, &servaddr.sin_addr);
	if(rc != 1) err("Incorrect IPaddress", EXIT);

	rc = connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	if(rc < 0) err("Could not connect to server", EXIT);
	
	printf("[CONNECTION ESTABLISHED]\n");

	unsigned char OK = ACK;
	const char* FIN = "FIN";
	char message_beyond[256];
	int established = 1;
	while(established) {
		printf("$ ");

		bzero(message_beyond, 256);
		if(!fgets(message_beyond, 256, stdin)){ err("read error", EXIT); };

		int bytes_sent;
		bytes_sent = send(sockfd, message_beyond,strlen(message_beyond) , 0); 
		if(bytes_sent == 0){
			printf("Server sent nothing, closing.");
			close(sockfd);
			exit(0);
		}

		recv(sockfd, &OK, sizeof(OK), 0);

	}
	close(sockfd);


	return 0;
}
