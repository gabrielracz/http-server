#ifndef CONTENT_H
#define CONTENT_H
#include "http.h"

void content_init();
void content_read_file(HttpRequest* rq, HttpResponse* res);
void content_error(HttpRequest* rq, HttpResponse* res);
void content_not_found(HttpRequest* rq, HttpResponse* res);
void content_perlin(HttpRequest* rq, HttpResponse* res);
void content_archiver(HttpRequest* rq, HttpResponse* res);

#endif