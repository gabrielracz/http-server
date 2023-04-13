#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
#include <netinet/in.h>
#include "../http-parser/picohttpparser.h"
#include <stdbool.h>

#define RESPONSE_BUFFER_SIZE 1024 * 1024
#define HEADER_BUFFER_SIZE 8192

typedef struct {
    char* ptr;
    size_t len;
    size_t size;
} Buffer;

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
    HTTP_SERVER_ERROR
};

enum HttpRoute {
    ROUTE_DISK = 0,
    ROUTE_ERROR,
    ROUTE_PERLIN,
    ROUTE_SPOTIFY_ARCHIVER,
    ROUTE_N_ROUTES
};

typedef struct  {
    StringView key;
    StringView value;
} HttpVariable;

typedef struct {
    enum HttpError err;
    char content_type[64];
    Buffer header;
    Buffer body;
    struct msghdr msg;
    struct iovec iov[2];    //vector of io blocks to be stitched together by 'sendmsg()'
} HttpResponse;

typedef struct
{
    enum HttpMethod method;
    enum HttpRoute route;
    HttpVariable variables[16];

    StringView raw_request;
    StringView path;
    StringView body;

    bool translated;
    char translated_path[256];

    struct phr_header headers[100];
    size_t num_headers;

    int minor_version;
    int major_version;

    bool parsed;
    bool done;

    char addr[INET_ADDRSTRLEN];
} HttpRequest;

int min(int a, int b);

int max(int a, int b);

int http_init();
HttpResponse* http_create_response();
HttpRequest* http_create_request(Buffer request_buffer, const char* client_address);
void http_destroy_request(HttpRequest* rq);
void http_destroy_response(HttpResponse* res);

void http_handle_request(HttpRequest* rq, HttpResponse* res);
void http_free(HttpRequest* rq);

const char* http_status_code(HttpResponse* res);

#endif