#include <string.h>
#include <stdio.h>
#include "http.h"
#include "logger.h"
#include "content.h"


void content_read_file(HttpRequest* rq, HttpResponse* res)
{
    FILE *fp;
    char path[1024] = "resources/";
    strncat(path, rq->path.ptr, rq->path.len);

    fp = fopen(path, "r");
    if (fp == NULL) {
        log_perror("http_read_file, could not open requested file");
        res->err = HTTP_NOT_FOUND;
        content_error(rq, res);
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t filelen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filelen > res->body.size) {
        //Transfer encoding chunked support goes here
        res->err = HTTP_NOT_FOUND;
        log_error("http_read_file error. filelen:%zu exceed buflen:%zu", filelen, res->body.size);
        fclose(fp);
        return;
    }

    size_t bytes_read;
    bytes_read = fread(res->body.ptr, 1, filelen, fp);
    res->body.ptr[bytes_read] = '\0';
    res->body.len = bytes_read;
    rq->done = true;
    fclose(fp);
    return;
}

void content_error(HttpRequest* rq, HttpResponse* res) {
    const char* status_code_str = http_status_code(res);
    res->body.len = sprintf(res->body.ptr,
           "<html>"
           "<head><title>%s</title><link rel=\"stylesheet\" href=\"/style.css\"></head>"
           "<body>"
           "<center><h1>%s</h1></center>"
           "<hr><center>naoko/2.0</center>"
           "</body>"
           "</html>",
           status_code_str,
           status_code_str);
    strcpy(res->content_type, "text/html");
    res->body.len = strlen(res->body.ptr);
}

// void content_not_found(HttpRequest *rq, HttpResponse *res) {

// }