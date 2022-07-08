#ifndef HTTP_H
#define HTTP_H
#include<stdio.h>
#include<strings.h>
#include<unistd.h>
#include<memory.h>
#include<stdint.h>

#include "universal.h"

typedef struct {
	const char* method;
	size_t methodlen;
	const char* path;
	size_t pathlen;
	struct phr_header headers[100];
	size_t num_headers;
	int minor_version;

} HTTP_req;

const char http_header_template[] = "\
HTTP/1.1 200 OK\r\n\
Content-Length: 150\r\n\
Content-Type: text/html; charset=utf-8\r\n\
\r\n";
const int status_index = 9;
const int content_length_index = 33;

char* get_status_code(int code);
int http_parse_request(char* req, size_t size);
int build_http_response(HTTP_req* req, char* buffer, size_t buflen);
#endif
