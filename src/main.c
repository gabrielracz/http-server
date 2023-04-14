#include "server.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {
	long port = 7120;
	if(argc == 2) {
		char* endptr;
		port = strtol(argv[1], &endptr, 10);
	}
	srand(time(NULL));
	server_on(port);
	return 0;
}
