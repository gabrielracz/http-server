#ifndef CONTENT_H
#define CONTENT_H
#include "http.h"

#define MAX_FILE_SIZE 10 * 1024 * 1024 * 1024

void content_init();
size_t content_read_file(HttpRequest* rq, HttpResponse* res);
size_t content_error(HttpRequest* rq, HttpResponse* res);
size_t content_not_found(HttpRequest* rq, HttpResponse* res);
size_t content_perlin(HttpRequest* rq, HttpResponse* res);
size_t content_archiver(HttpRequest* rq, HttpResponse* res);
size_t content_sha256(HttpRequest* rq, HttpResponse* res);
size_t content_stats(HttpRequest* rq, HttpResponse* res);

#endif