#ifndef HTTP_H
#define HTTP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <netinet/in.h> //sockaddr_in, htons, INADDR_ANY
#include <ctype.h>
#include <stdbool.h>

#include "../http-parser/picohttpparser.h"
#include "perlin.h"
#include "logger.h"

static inline int min(int a, int b) {
    return a < b ? a : b;
}

static inline int max(int a, int b) {
    return a > b ? a : b;
}

enum HttpMethod {
    GET,
    POST,
    PUT,
    DELETE
};

typedef struct {
    char* ptr;
    size_t len;
    size_t size;
} Buffer;

typedef struct {
    const char* ptr;
    size_t len;
} StringView;

enum HttpRoute {
    DISK = 0,
    PERLIN,
    SPOTIFY_ARCHIVER,
    N_ROUTES
};

enum HttpError {
    OK,
    NOT_FOUND,
    BAD_REQUEST,
    FORBIDDEN
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
    HttpMethod method;
    HttpRoute route;
    HttpError err;
    HttpVariable variables[16];

    StringView path;
    StringView body;

    Buffer output;

    struct phr_header headers[100];
    size_t num_headers;

    int minor_version;
    int major_version;

    char addr[INET_ADDRSTRLEN];
} HttpRequest;


void http_validate(HttpRequest* rq) {
    //check that rq.path is valid (ex. no '..' no leading '.' or invalid characters)
    bool is_valid = true;
    if(is_valid) {
        rq->err = OK;
    }else {
        rq->err = BAD_REQUEST;
    }
}

typedef struct {
    const char* path;
    HttpRoute route;
} HttpRouteEntry;

static const HttpRouteEntry routes[] = {
    {"perlin", PERLIN},
    {"spotify", SPOTIFY_ARCHIVER}
};

void http_route(HttpRequest* rq) {
    int n_routes = sizeof(routes)/sizeof(routes[0]);
    for(int i = 0; i < n_routes; i++) {
        int route_len = sizeof(routes[i].path);
        if(strncmp(rq->path.ptr, routes[i].path, max(rq->path.len, route_len))) {
            rq->route = routes[i].route;
            return;
        }
    }
    rq->route = DISK;  //default to serving file off disk
}

void http_fill_body(HttpRequest* rq, HttpResponse* res) {
    switch(rq->route) {
        case DISK:
            break;
        case PERLIN:
            break;
        case SPOTIFY_ARCHIVER:
            break;
        default:
            break;
    }
}

void http_format(HttpRequest* rq, HttpResponse* res) {
    res->msg = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    /* Generate the headers */
    res->header.ptr = (char*) malloc(64);
    res->header.len = 64;
    for(int i = 0; i < 64; i++) {
        res->header.ptr[i] = 'c';
    }

    /* We can use sendmsg() to send the seperate header and body buffers in a single call */
    struct iovec iov[2] = {
        {.iov_base = res->header.ptr, .iov_len = res->header.len},
        {.iov_base = res->body.ptr, .iov_len = res->body.len }
    };
    res->msg.msg_iov = iov;
    res->msg.msg_iovlen = 2;
}



HttpResponse http_handle_request(HttpRequest* rq) {
    HttpResponse res;
    http_validate(rq);
    http_route(rq);
    http_fill_body(rq, &res);
    http_format(rq, &res);
    return res;
}

#endif