#ifndef UTIL_H
#define UTIL_H
#include "stdlib.h"

#define max(a, b) (a)>(b) ? (a) : (b)
#define min(a, b) (a)<(b) ? (a) : (b)
#define randf(lim) (((float)rand()/(float)(RAND_MAX)) * lim)

#endif