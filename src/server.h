#ifndef SERVER_H
#define SERVER_H
#include <sys/types.h>

void server_on(int port);
void server_get_stats(size_t* bytes, size_t* requests);
void server_get_virtual_memory(size_t* mem);
const char* server_get_start_time();
#endif
