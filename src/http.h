#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
#include <netinet/in.h>
#include "../http-parser/picohttpparser.h"

typedef struct {
    char* ptr;
    size_t len;
    size_t size;
} Buffer;

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
    HTTP_METHOD_NOT_ALLOWED
};

enum HttpRoute {
    DISK = 0,
    PERLIN,
    SPOTIFY_ARCHIVER,
    N_ROUTES
};

typedef struct  {
    StringView key;
    StringView value;
} HttpVariable;

typedef struct {
    Buffer header;
    Buffer body;
    struct msghdr msg;
}HttpResponse;

typedef struct
{
    enum HttpMethod method;
    enum HttpRoute route;
    enum HttpError err;
    HttpVariable variables[16];

    StringView raw_request;
    StringView path;
    StringView body;

    Buffer output;

    struct phr_header headers[100];
    size_t num_headers;
    char content_type[64];

    int minor_version;
    int major_version;

    char addr[INET_ADDRSTRLEN];
} HttpRequest;

inline int min(int a, int b) {
    return a < b ? a : b;
}

inline int max(int a, int b) {
    return a > b ? a : b;
}

int http_init();
void http_parse(HttpRequest* rq);
HttpResponse http_handle_request(HttpRequest* rq);
void http_free(HttpRequest* rq);

#endif