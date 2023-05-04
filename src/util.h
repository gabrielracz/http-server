#ifndef UTIL_H
#define UTIL_H
#include "stdlib.h"

#define SERVER_NAME "naoko/2.2"

#define max(a, b) (a)>(b) ? (a) : (b)
#define min(a, b) (a)<(b) ? (a) : (b)
#define randf(lim) (((float)rand()/(float)(RAND_MAX)) * lim)

#endif