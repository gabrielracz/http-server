#ifndef HTTP_H
#define HTTP_H
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<memory.h>
#include<stdint.h>
#include<netinet/in.h> //sockaddr_in, htons, INADDR_ANY
#include<errno.h>

#include "universal.h"


typedef struct {
	const char* method;
	size_t methodlen;
	const char* path;
	size_t pathlen;
	struct phr_header headers[100];
	size_t num_headers;
	int minor_version;

	struct sockaddr_in addr;
	char addr_str[INET_ADDRSTRLEN];

} HTTPrq;

const char* http_min_res = "\nHTTP/1.1 200 OK\nContent-Length: 13\nContent-Type: text/plain; charset=utf-8\n\nHello World!";
const char* http_head_ok = "\nHTTP/1.1 200 OK\nContent-Length: 150\nContent-Type: text/plain; charset=utf-8\n\n";

char* http_status_code(int code);
int http_parse_request(char* req, size_t size);
size_t http_handle_rq(HTTPrq rq, char* resbuf, size_t reslen);
int http_parse(char* buffer, size_t buflen, HTTPrq* rq);
int http_sanitize_rq(HTTPrq* rq, char* filename, size_t len);

int http_parse(char* buffer, size_t buflen, HTTPrq* rq){
	/*parse request*/
	rq->num_headers = 100;
	int prc;
	prc = phr_parse_request(buffer, buflen, &rq->method, &rq->methodlen, 
			&rq->path, &rq->pathlen, &rq->minor_version, rq->headers, &rq->num_headers, 0);

	if(prc < 0){
		printf("HTTP parse error: %d\n", prc);
	pthread_exit(0);
	}

	//headers
	//size_t h0namelen = rq->headers[0].name_len;
	//char* h0name = malloc(h0namelen + 1);
	//strncpy(h0name, rq->headers[0].name, h0namelen);
	//h0name[h0namelen] = '\0';

	//size_t h0valuelen = rq->headers[0].value_len;
	//char* h0value = malloc(h0valuelen + 1);
	//strncpy(h0value, rq->headers[0].value, h0valuelen);
	//h0value[h0valuelen] = '\0';

	//printf("%s : %s\n", h0name, h0value);

	////for(int i = 0; i < rq->num_headers; i++){
		////printf("%*s - %*s\n", (int)rq->headers[i].name_len, rq->headers[i].name, (int)rq->headers[i].value_len, rq->headers[i].value);
	////}

	return prc;
}

size_t read_file(const char* filename, char* buf, size_t buflen) {
	FILE* fp;
	size_t filelen;
	fp = fopen(filename, "r");
	if(fp == 0){
		printf("Couldn't open requested file. errno: %d\n", errno);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	filelen	= ftell(fp);
	rewind(fp);

	if(filelen > buflen) {
		printf("http_read_file error. filelen:%zu exceed buflen:%zu\n", filelen, buflen);
		fclose(fp);
		return 0;
	}

	size_t bytes_read;
	bytes_read = fread(buf, 1, filelen, fp);
	buf[filelen] = '\0';
	fclose(fp);
	return filelen;
}

typedef struct {
	const char* suffix;
	uint8_t suffix_size;
	const char* mime_type;
} mime_type;

static const mime_type filetypes[] = {
    { "c",		1,	"text/plain"},
    { "cc",		1,	"text/plain"},
    { "cpp",    1,	"text/plain"},
    { "css",	3, "text/css"},
    { "h",		1,	"text/plain"},
    { "hh",		1,	"text/plain"},
    { "html",	4,	"text/html; charset=utf-8"},
	{ "ico",	3,	"image/x-icon"},
    { "jpg",	3,	"image/jpeg"},
    { "png",	3,	"image/png"},
    { "txt",    1,	"text/plain"},
};

int http_get_content_type(const char* filename, size_t len) {
	//search from behind until hit a period.
	//binary search the fieltypes
	//return the mime type for the request.
	
	//get the file type
	//
	int first = 0;
	int last = sizeof(filetypes) / sizeof(filetypes[0]);
	int t;
	for(t = len-1; t>0 && filename[t] != '.'; t--) {}
	//i is the index of the dot
	const char* inp = &filename[t+1];
	int cmp;
	int i;
	while(first <= last) {
		i = (first+last)/2;
		cmp = strcmp(inp, filetypes[i].suffix);
		if(cmp == 0) {
			return i;
		} else if(cmp < 1) {
			last = i - 1;
		} else {
			first = i + 1;
		}
	}
	return -1;
}

int http_response(const char* path, size_t pathlen, char* buf, size_t buflen) {
	char header[1024];
	size_t content_len = 400;
	int status_code = 200;
	printf("[%zu]\n", buflen);

	//read file
	size_t filelen;
	char* message = calloc(buflen - 200, 1);
	//make a 404() function
	if((filelen = read_file(path, message, buflen-200)) == 0) {		//Couldn't open requested file
		status_code = 404;
		filelen = read_file("resources/404.html", message, buflen-200);
	}

	/*construct header*/
	int mimdex = http_get_content_type(path, pathlen);
	const char* content_type;
	if(mimdex < 0) {
		printf("unsupported content-type request\n");
		content_type = "text/plain";

	} else {
		content_type = filetypes[mimdex].mime_type;
		printf("%s %d\n", content_type, mimdex);
	}

	sprintf(header, 
		"HTTP/1.1 %s\r\n"
		"Content-Length: %ld\r\n"
		"Content-Type: %s;\r\n\r\n",
		http_status_code(status_code),
		filelen,
		content_type
	);
	size_t headerlen = strlen(header);
	size_t responselen = headerlen + filelen + 1;
	if(responselen > buflen){
		printf("response len exceeds buffer len\n");
		return -1;
	}


	memcpy(buf, header, headerlen);
	memcpy(buf + headerlen, message, filelen + 1);
	return responselen;
}

size_t http_handle_rq(HTTPrq rq, char* resbuf, size_t reslen) {
	printf("HTTP request received:   %s:%d\n", rq.addr_str, rq.addr.sin_port);
	printf("method: %.*s\n", (int) rq.methodlen, rq.method);
	printf("path: %.*s\n", (int) rq.pathlen, rq.path);
	printf("\n");

	//TODO: sanitize path
	/*http_sanitize_rq()*/
	char path[1024] = "resources";
	strncat(path, rq.path, rq.pathlen);

	//big if block for each http route
	
	size_t len = http_response(path, strlen(path), resbuf, reslen);

	printf("[RESPONSE]\n%s\n", resbuf);

	return len;
}

char* http_status_code(int code){
	switch(code){
		case 200: return "200 OK";
		case 404: return "404 Not Found";
		case 500: return "500 Server Error";
		case 503: return "503 Service Unavailable";
	}
	return NULL;
}
#endif //HTTP_H
