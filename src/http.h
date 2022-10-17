#ifndef HTTP_H
#define HTTP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>
#include <netinet/in.h> //sockaddr_in, htons, INADDR_ANY
#include <errno.h>
#include <pthread.h>

#include "../http-parser/picohttpparser.h"
#include "perlin.h"
#include "logger.h"

typedef struct
{
    const char *method;
    size_t methodlen;
    const char *path;
    size_t pathlen;
    struct phr_header headers[100];
    size_t num_headers;
    int minor_version;

    struct sockaddr_in addr;
    char addr_str[INET_ADDRSTRLEN];

} HTTPrq;

static const char *http_min_res = "\nHTTP/1.1 200 OK\nContent-Length: 13\nContent-Type: text/plain; charset=utf-8\n\nHello World!";
static const char *http_head_ok = "\nHTTP/1.1 200 OK\nContent-Length: 150\nContent-Type: text/plain; charset=utf-8\n\n";

const char* http_status_code(int code);
int http_build_header(char* header, size_t header_size, int status_code, size_t content_len, const char* content_type, char** additional, int num_adds);
int http_parse_request(char *req, size_t size);
size_t http_handle_rq(HTTPrq rq, char *resbuf, size_t rslen);
int http_parse(char *buffer, size_t buflen, HTTPrq *rq);
int http_sanitize_rq(HTTPrq *rq, char *filename, size_t len);

int http_init()
{
	 perlinit(rand());
}

int http_parse(char *buffer, size_t buflen, HTTPrq *rq)
{
    /*parse request*/
    rq->num_headers = 100;
    int prc;
    prc = phr_parse_request(buffer, buflen, &rq->method, &rq->methodlen,
                            &rq->path, &rq->pathlen, &rq->minor_version, rq->headers, &rq->num_headers, 0);

    if (prc < 0){
        log_error("HTTP parse error: %d", prc);
        pthread_exit(0);
    }

    // headers
    // size_t h0namelen = rq->headers[0].name_len;
    // char* h0name = malloc(h0namelen + 1);
    // strncpy(h0name, rq->headers[0].name, h0namelen);
    // h0name[h0namelen] = '\0';

    // size_t h0valuelen = rq->headers[0].value_len;
    // char* h0value = malloc(h0valuelen + 1);
    // strncpy(h0value, rq->headers[0].value, h0valuelen);
    // h0value[h0valuelen] = '\0';

    // printf("%s : %s\n", h0name, h0value);

    ////for(int i = 0; i < rq->num_headers; i++){
    ////printf("%*s - %*s\n", (int)rq->headers[i].name_len, rq->headers[i].name, (int)rq->headers[i].value_len, rq->headers[i].value);
    ////}

    return prc;
}

size_t read_file(const char *filename, char *buf, size_t buflen)
{
    FILE *fp;
    size_t filelen;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        log_perror("http_read_file, could not open requested file");
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp);
    rewind(fp);

    if (filelen > buflen)
    {
        log_error("http_read_file error. filelen:%zu exceed buflen:%zu", filelen, buflen);
        fclose(fp);
        return 0;
    }

    size_t bytes_read;
    bytes_read = fread(buf, 1, filelen, fp);
    buf[filelen] = '\0';
    fclose(fp);
    return filelen;
}

typedef struct
{
    const char *suffix;
    const char *mime_type;
} mime_type;

static const mime_type filetypes[] = {
    {"c", "text/plain"},
    {"cc", "text/plain"},
    {"cpp", "text/plain"},
    {"css", "text/css"},
    {"flac", "audio/mpeg"},
    {"h", "text/plain"},
    {"hh", "text/plain"},
    {"html", "text/html; charset=utf-8"},
    {"ico", "image/x-icon"},
    {"jpg", "image/jpeg"},
    {"mp3", "audio/mpeg"},
    {"pdf", "application/pdf"},
    {"png", "image/png"},
    {"tar", "application/x-tar"},
    {"txt", "text/plain"}};

int http_get_content_type(const char *filename, size_t len)
{
    //get . index and binary search the filetypes for corresponding mime_type
    int first = 0;
    int last = sizeof(filetypes) / sizeof(filetypes[0]);
    int t;
    for (t = len - 1; t > 0 && filename[t] != '.'; t--)
    {
    }
    // i is the index of the dot
    const char *inp = &filename[t + 1];
    int cmp;
    int i;
    while (first <= last)
    {
        i = (first + last) / 2;
        cmp = strcmp(inp, filetypes[i].suffix);
        if (cmp == 0)
        {
            return i;
        }
        else if (cmp < 1)
        {
            last = i - 1;
        }
        else
        {
            first = i + 1;
        }
    }
    return -1;
}

size_t http_not_found(char *buf)
{
    const char *not_found =
        "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-type: text/html\r\n\r\n"
        "<head><title>404</title></head>\n"
        "<body>Document Not Found</body>\n";

    strcpy(buf, not_found);
    return strlen(buf);
}

struct static_buffer {
    const char* data;
    size_t size;
};

static const int default_header_size = 8192;
int http_build_header(char* header, size_t header_size, int status_code, size_t content_len, const char* content_type, char** options, int num_opts) {


    sprintf(header,
            "HTTP/1.1 %s\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: %s;\r\n",
            http_status_code(status_code),
            content_len,
            content_type);

	int n = strlen(header);
	char* c = &header[n];

	//append all options to header followed by an extra \r\n
	for(int i = 0; i < num_opts; i++) {
		int opt_size = strlen(options[i]);
		n += opt_size + 2;
		if(n >= header_size) {
			log_error("Header options would overflow buffer, discarding past %s", options[i]);
			break;
		}
		memcpy(c, options[i], opt_size);
		c += opt_size;
		*(c++) = '\r';
		*(c++) = '\n';
	}

	*(c++) = '\r';
	*(c++) = '\n';
	n += 2;

	return n;
}

const char* resources_dir = "resources";
#define RESOURCES "resources"
#define randf(lim) (((float)rand()/(float)(RAND_MAX)) * lim)
int http_serv_perlin(char *buf, size_t buflen)
{

    char header[default_header_size];
    size_t content_len = 400;
    int status_code = 200;
    const char *content_type = "text/html; charset=utf-8";

	const char* html_head = 
		"<!DOCTYPE html>"
		"<html lang=\"en\">"
		"<head>"
        "<meta charset=\"UTF-8\">"
		"<title>perlin</title>"
        "<link rel=\"stylesheet\" href=\"/style.css\">"
		"</head>"
		"<body>\n"
		"<p style=\"white-space: pre;\"> \n";
	const int html_head_len = strlen(html_head);
		
	//FIXME UNSAFE

    size_t len;
	int w = 250;
	int h = 100;
	size_t n = PGRIDSIZE(w,h);
	char* perlin_contents = (char*) malloc(n);
	len = perlin_sample_grid(perlin_contents, n, w, h, 
						randf(200.0f), randf(200.0f), randf(0.19f));

	char* options[] = {"Connection: close"};
	size_t headerlen = http_build_header(header, default_header_size, 200, len, content_type, options, 1);
    size_t responselen = headerlen + html_head_len + len + 1;
    if (responselen > buflen) {
        log_error("response len %zu exceeds buffer len %zu", responselen, buflen);
		free(perlin_contents);
        return 0;
    }

    memcpy(buf, header, headerlen);
	strcpy(buf + headerlen, html_head);
    memcpy(buf + headerlen + html_head_len, perlin_contents, len + 1);
	free(perlin_contents);
    return responselen;
}

int http_serv_file(const char *path, size_t pathlen, char *buf, size_t buflen)
{

    char header[default_header_size];
    size_t content_len = 400;
    int status_code = 200;
    const char *content_type;

    size_t filelen;
    char* file_contents = (char*) calloc(buflen, 1);
    if ((filelen = read_file(path, file_contents, buflen - 200)) == 0) { // Couldn't open requested file
		filelen = read_file( RESOURCES"/404.html", file_contents, buflen - 200);
		log_info("404 for %s", path);
		content_type = "text/html";
		status_code = 404;
        //return http_not_found(buf);
    }else {
        int mimdex = http_get_content_type(path, pathlen);
        if (mimdex < 0){
			log_error("unsupported content-type request for %s", path);
            content_type = "text/plain";
        }
        else{
            content_type = filetypes[mimdex].mime_type;
            //printf("%s %d\n", content_type, mimdex);
        }
    }

	char* options[] = {"Connection: close"};
	//char* options[] = {"Connection: close", "Transfer-Encoding: chunked"};
	size_t headerlen = http_build_header(header, default_header_size, status_code, filelen, content_type, options, sizeof(options)/sizeof(options[0]));
    size_t responselen = headerlen + filelen + 1;
    if (responselen > buflen) {
        log_error("response len exceeds buffer len");
        free(file_contents);
        return 0;
    }

    memcpy(buf, header, headerlen);
    memcpy(buf + headerlen, file_contents, filelen + 1);
    free(file_contents);
    return responselen;
}

enum ResponseTypes {
    DISK,
    PERLIN
};

size_t http_handle_rq(HTTPrq rq, char *resbuf, size_t reslen) {
    log_info("HTTP request received:  %s:%d", rq.addr_str, rq.addr.sin_port);
    log_info("Method:                 %.*s", (int)rq.methodlen, rq.method);
    log_info("Path:                   %.*s", (int)rq.pathlen, rq.path);

    // TODO: sanitize path
    /*http_sanitize_rq()*/
    char rqfile[1024] = "resources";


    int route = DISK;
    if (strncmp("/", rq.path, rq.pathlen) == 0 || strncmp("/index.html", rq.path, rq.pathlen) == 0){
        strcat(rqfile, "/gsr.html");
    }
    else if (strncmp("/perlin.html", rq.path, rq.pathlen) == 0){
        route = PERLIN;
    }
    else{
        strncat(rqfile, rq.path, rq.pathlen);
    }

    size_t len;
    switch(route) {
        case DISK:
            len = http_serv_file(rqfile, strlen(rqfile), resbuf, reslen);
            break;
        case PERLIN:
			len = http_serv_perlin(resbuf, reslen);
            break;
        break;
    default:
        break;
    }

	//printf("[RESPONSE]\n%s\n", resbuf);
    return len;
}

const char* http_status_code(int code)
{
    switch (code)
    {
    case 200:
        return "200 OK";
    case 404:
        return "404 Not Found";
    case 500:
        return "500 Server Error";
    case 503:
        return "503 Service Unavailable";
    }
    return NULL;
}
#endif // HTTP_H
