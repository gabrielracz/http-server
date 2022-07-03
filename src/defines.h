#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include<sys/socket.h>
#include<sys/types.h>  //pid_t
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY
#include<memory.h>
#include<unistd.h>

#define ERR_FILE stdout
#define SERV_PORT 3000
#define ACK 200;
#define SERV_ADDR INADDR_ANY

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
