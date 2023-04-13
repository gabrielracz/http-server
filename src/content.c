#include <string.h>
#include <stdio.h>
#include "http.h"
#include "logger.h"


void content_read_file(HttpRequest* rq, HttpResponse* res)
{
    FILE *fp;
    char path[1024] = "resources";
    strncat(path, rq->path.ptr, rq->path.len);
    fp = fopen(path, "r");
    if (fp == NULL) {
        log_perror("http_read_file, could not open requested file");
        rq->err = HTTP_NOT_FOUND;
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t filelen = ftell(fp);
    rewind(fp);

    if (filelen > res->body.size) {
        //Transfer encoding chunked support goes here
        rq->err = HTTP_NOT_FOUND;
        log_error("http_read_file error. filelen:%zu exceed buflen:%zu", filelen, res->body.size);
        fclose(fp);
        return;
    }

    size_t bytes_read;
    bytes_read = fread(res->body.ptr, 1, filelen, fp);
    res->body.ptr[res->body.len] = '\0';
    fclose(fp);
    return;
}

void content_not_found(HttpRequest *rq, HttpResponse *res) {

}