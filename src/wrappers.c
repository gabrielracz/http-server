#include "defines.h"

int Socket(int domain, int type, int protocol){
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		err("Socket initializtion error", EXIT);
	return sockfd;
}
