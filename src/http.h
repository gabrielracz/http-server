#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
#include <netinet/in.h>
#include "picohttpparser.h"
#include <stdbool.h>

#define RESPONSE_BUFFER_SIZE 30 * 1024 * 1024
#define HEADER_BUFFER_SIZE 8192
#define MAX_VARIABLES 16

typedef struct {
    char* ptr;
    size_t len;
    size_t size;
} Buffer;

typedef struct {
    char ptr[128];
    size_t len;
    size_t size;
} StaticVariableBuffer;

typedef struct {
    char ptr[RESPONSE_BUFFER_SIZE];
    size_t len;
    size_t size;
} StaticResponseBuffer;

typedef struct {
    char ptr[HEADER_BUFFER_SIZE];
    size_t len;
    size_t size;
} StaticHeaderBuffer;

typedef struct {
    const char* ptr;
    size_t len;
} StringView;


enum HttpMethod {
    GET,
    POST,
    PUT,
    DELETE
};

enum HttpError {
    HTTP_OK,
    HTTP_NOT_FOUND,
    HTTP_BAD_REQUEST,
    HTTP_FORBIDDEN,
    HTTP_METHOD_NOT_ALLOWED,
    HTTP_CONTENT_TOO_LARGE,
    HTTP_SERVER_ERROR
};

enum HttpRoute {
    ROUTE_DISK = 0,
    ROUTE_ERROR,
    ROUTE_PERLIN,
    ROUTE_SPOTIFY_ARCHIVER,
    ROUTE_SHA256,
    ROUTE_N_ROUTES
};

typedef struct  {
    StaticVariableBuffer key;
    StaticVariableBuffer value;
} HttpVariable;

enum ParseStatus {
    PARSE_HEADERS = 0,
    PARSE_BODY,
    PARSE_COMPLETE
};

enum RequestStatus {
    REQUEST_PROCESSING,
    REQUEST_RESPONDING,
    REQUEST_COMPLETE
};

typedef struct {
    enum HttpError err;
    char content_type[64];
    Buffer header;
    Buffer body;
    struct msghdr msg;
    struct iovec iov[2];    //vector of io blocks to be stitched together by 'sendmsg()'
} HttpResponse;

typedef struct {
    StringView raw_request;
    StringView path;
    StringView body;
    StringView method_str;
    size_t content_length;

    size_t target_output_length;
    size_t output_length;

    struct phr_header headers[100];
    size_t n_headers;
    HttpVariable variables[MAX_VARIABLES];
    size_t n_variables;

    enum HttpMethod method;
    enum HttpRoute route;
    enum ParseStatus parse_status;
    enum RequestStatus status;

    int minor_version;
    int major_version;
    char addr[INET_ADDRSTRLEN];

} HttpRequest;

HttpRequest* http_create_request(Buffer request_buffer, const char* client_address);
void http_destroy_request(HttpRequest* rq);

HttpResponse* http_create_response();
void http_destroy_response(HttpResponse* res);

void http_update_request_buffer(HttpRequest* rq, Buffer request_buffer);
void http_parse(HttpRequest* rq, HttpResponse* res);
void http_parse_body(HttpRequest* rq, HttpResponse* res);
void http_handle_request(HttpRequest* rq, HttpResponse* res);

const char* http_status_code(HttpResponse* res);

#endif
