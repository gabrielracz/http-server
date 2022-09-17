#ifndef UNIVERSAL_H
#define UNIVERSAL_H

#include<stdio.h>
#include<stdlib.h>
#include<strings.h>

#include<unistd.h>
#include<memory.h>
#include<errno.h>

#include<pthread.h>

#include<signal.h>
#include<sys/socket.h>
#include<sys/types.h>  //pid_t
#include<arpa/inet.h>  //inet_ptons
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY

#include "../phttp/picohttpparser.h"

#define ERR_FILE stdout
#define SERV_PORT 80
#define ACK 200;
#define SERV_ADDR INADDR_ANY

#define MAXLEN 1024

enum Terminate {
	EXIT,
	CONTINUE,
};

inline void err(const char* msg, enum Terminate term){
	fprintf(stdout, "%s\n", msg);
	switch(term){
		case EXIT:		exit(-1); break;
		case CONTINUE:	return;
	}
}

#endif
