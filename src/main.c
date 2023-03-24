#include "server.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {
	srand(time(NULL));
	server_on();
	return 0;
}
