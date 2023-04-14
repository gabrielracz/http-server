#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h> //sockaddr_in, htons, INADDR_ANY
#include <ctype.h>
#include <stdbool.h>

#include "http.h"
#include "content.h"
#include "logger.h"
#include "util.h"
#include "perlin.h"

const char index_path[] = "/gsr.html";

int http_init() {
    perlinit(1337);
    return 0;
}

HttpRequest* http_create_request(Buffer request_buffer, const char *client_address) {
    HttpRequest* rq = malloc(sizeof(HttpRequest));
    rq->raw_request.ptr = request_buffer.ptr;
    rq->raw_request.len = request_buffer.len;
    strncpy(rq->addr, client_address, INET_ADDRSTRLEN);

    rq->parsed = false;
    rq->done = false;
    rq->translated = false;
    return rq;
}

void http_destroy_request(HttpRequest* rq) {
    free(rq);
}

HttpResponse* http_create_response() {
    HttpResponse* res = malloc(sizeof(HttpResponse));
    res->body.ptr = malloc(RESPONSE_BUFFER_SIZE);
    res->body.size = RESPONSE_BUFFER_SIZE;
    res->body.len = 0;

    res->header.ptr = malloc(HEADER_BUFFER_SIZE);
    res->header.size = HEADER_BUFFER_SIZE;
    res->header.len = 0;
    return res;
}

void http_destroy_response(HttpResponse* res) {
    free(res->body.ptr);
    free(res->header.ptr);
    free(res);
}

static void http_translate_path(HttpRequest* rq) {
    if(rq->path.len == 1 && strncmp("/"          , rq->path.ptr, rq->path.len) == 0 ||
       rq->path.len == 11 && strncmp("/index.html", rq->path.ptr, rq->path.len) == 0) {
        rq->path.ptr = index_path;
        rq->path.len = sizeof(index_path)-1;
    }
}

static const struct {
    const char* suffix;
    const int size;
    const char* mime;
    const int mime_size;
} content_types[] = {
    { "ai",         2, "application/postscript"                 , sizeof("application/postscript"      )},
    { "aif",        3, "audio/x-aiff"                           , sizeof("audio/x-aiff"                )},
    { "aifc",       4, "audio/x-aiff"                           , sizeof("audio/x-aiff"                )},
    { "aiff",       4, "audio/x-aiff"                           , sizeof("audio/x-aiff"                )},
    { "apk",        3, "application/vnd.android.package-archive", sizeof("application/vnd.android.package-archive"                )},
    { "asc",        3, "text/plain"                             , sizeof("text/plain"                  )},
    { "asf",        3, "video/x-ms-asf"                         , sizeof("video/x-ms-asf"              )},
    { "asx",        3, "video/x-ms-asx"                         , sizeof("video/x-ms-asx"              )},
    { "au",         2, "audio/ulaw"                             , sizeof("audio/ulaw"                  )},
    { "avi",        3, "video/x-msvideo"                        , sizeof("video/x-msvideo"             )},
    { "bat",        3, "application/x-msdos-program"            , sizeof("application/x-msdos-program" )},
    { "bcpio",      5, "application/x-bcpio"                    , sizeof("application/x-bcpio"         )},
    { "bin",        3, "application/octet-stream"               , sizeof("application/octet-stream"    )},
    { "c",          1, "text/plain"                             , sizeof("text/plain"                  )},
    { "cc",         2, "text/plain"                             , sizeof("text/plain"                  )},
    { "cpio",       4, "application/x-cpio"                     , sizeof("application/x-cpio"          )},
    { "css",        3, "text/css"                               , sizeof("text/css"                    )},
    { "deb",        3, "application/x-debian-package"           , sizeof("application/x-debian-package")},
    { "dl",         2, "video/dl"                               , sizeof("video/dl"                    )},
    { "dms",        3, "application/octet-stream"               , sizeof("application/octet-stream"    )},
    { "doc",        3, "application/msword"                     , sizeof("application/msword"          )},
    { "eps",        3, "application/postscript"                 , sizeof("application/postscript"      )},
    { "exe",        3, "application/octet-stream"               , sizeof("application/octet-stream"    )},
    { "gif",        3, "image/gif"                              , sizeof("image/gif"                   )},
    { "gl",         2, "video/gl"                               , sizeof("video/gl"                    )},
    { "gtar",       4, "application/x-gtar"                     , sizeof("application/x-gtar"          )},
    { "gz",         2, "application/x-gzip"                     , sizeof("application/x-gzip"          )},
    { "hh",         2, "text/plain"                             , sizeof("text/plain"                  )},
    { "h",          1, "text/plain"                             , sizeof("text/plain"                  )},
    { "htm",        3, "text/html; charset=utf-8"               , sizeof("text/html; charset=utf-8"    )},
    { "html",       4, "text/html; charset=utf-8"               , sizeof("text/html; charset=utf-8"    )},
    { "ico" ,       3, "image/x-icon"                           , sizeof("image/x-icon"                )},
    { "jar",        3, "application/java-archive"               , sizeof("application/java-archive"    )},
    { "jpeg",       4, "image/jpeg"                             , sizeof("image/jpeg"                  )},
    { "jpe",        3, "image/jpeg"                             , sizeof("image/jpeg"                  )},
    { "jpg",        3, "image/jpeg"                             , sizeof("image/jpeg"                  )},
    { "js",         2, "application/x-javascript"               , sizeof("application/x-javascript"    )},
    { "latex",      5, "application/x-latex"                    , sizeof("application/x-latex"         )},
    { "lha",        3, "application/octet-stream"               , sizeof("application/octet-stream"    )},
    { "lsp",        3, "application/x-lisp"                     , sizeof("application/x-lisp"          )},
    { "lzh",        3, "application/octet-stream"               , sizeof("application/octet-stream"    )},
    { "mid",        3, "audio/midi"                             , sizeof("audio/midi"                  )},
    { "midi",       4, "audio/midi"                             , sizeof("audio/midi"                  )},
    { "mime",       4, "www/mime"                               , sizeof("www/mime"                    )},
    { "movie",      5, "video/x-sgi-movie"                      , sizeof("video/x-sgi-movie"           )},
    { "mov",        3, "video/quicktime"                        , sizeof("video/quicktime"             )},
    { "mp2",        3, "audio/mpeg"                             , sizeof("audio/mpeg"                  )},
    { "mp2",        3, "video/mpeg"                             , sizeof("video/mpeg"                  )},
    { "mp3",        3, "audio/mpeg"                             , sizeof("audio/mpeg"                  )},
    { "mp4",        3, "video/mpeg"                             , sizeof("video/mpeg"                  )},
    { "mpeg",       4, "video/mpeg"                             , sizeof("video/mpeg"                  )},
    { "mpe",        3, "video/mpeg"                             , sizeof("video/mpeg"                  )},
    { "mpga",       4, "audio/mpeg"                             , sizeof("audio/mpeg"                  )},
    { "mpg",        3, "video/mpeg"                             , sizeof("video/mpeg"                  )},
    { "oda",        3, "application/oda"                        , sizeof("application/oda"             )},
    { "ogg",        3, "application/ogg"                        , sizeof("application/ogg"             )},
    { "ogm",        3, "application/ogg"                        , sizeof("application/ogg"             )},
    { "pdf",        3, "application/pdf"                        , sizeof("application/pdf"             )},
    { "pgm",        3, "image/x-portable-graymap"               , sizeof("image/x-portable-graymap"    )},
    { "pgp",        3, "application/pgp"                        , sizeof("application/pgp"             )},
    { "pl",         2, "application/x-perl"                     , sizeof("application/x-perl"          )},
    { "pm",         2, "application/x-perl"                     , sizeof("application/x-perl"          )},
    { "png",        3, "image/png"                              , sizeof("image/png"                   )},
    { "pnm",        3, "image/x-portable-anymap"                , sizeof("image/x-portable-anymap"     )},
    { "pot",        3, "application/mspowerpoint"               , sizeof("application/mspowerpoint"    )},
    { "pps",        3, "application/mspowerpoint"               , sizeof("application/mspowerpoint"    )},
    { "ppt",        3, "application/mspowerpoint"               , sizeof("application/mspowerpoint"    )},
    { "ppz",        3, "application/mspowerpoint"               , sizeof("application/mspowerpoint"    )},
    { "ps",         2, "application/postscript"                 , sizeof("application/postscript"      )},
    { "qt",         2, "video/quicktime"                        , sizeof("video/quicktime"             )},
    { "ra",         2, "audio/x-realaudio"                      , sizeof("audio/x-realaudio"           )},
    { "ram",        3, "audio/x-pn-realaudio"                   , sizeof("audio/x-pn-realaudio"        )},
    { "rar",        3, "application/x-rar-compressed"           , sizeof("application/x-rar-compressed")},
    { "ras",        3, "image/cmu-raster"                       , sizeof("image/cmu-raster"            )},
    { "ras",        3, "image/x-cmu-raster"                     , sizeof("image/x-cmu-raster"          )},
    { "rgb",        3, "image/x-rgb"                            , sizeof("image/x-rgb"                 )},
    { "rtf",        3, "application/rtf"                        , sizeof("application/rtf"             )},
    { "rtf",        3, "text/rtf"                               , sizeof("text/rtf"                    )},
    { "rtx",        3, "text/richtext"                          , sizeof("text/richtext"               )},
    { "sgml",       4, "text/sgml"                              , sizeof("text/sgml"                   )},
    { "sgm",        3, "text/sgml"                              , sizeof("text/sgml"                   )},
    { "sh",         2, "application/x-sh"                       , sizeof("application/x-sh"            )},
    { "shar",       4, "application/x-shar"                     , sizeof("application/x-shar"          )},
    { "snd",        3, "audio/basic"                            , sizeof("audio/basic"                 )},
    { "tar",        3, "application/x-tar"                      , sizeof("application/x-tar"           )},
    { "tcl",        3, "application/x-tcl"                      , sizeof("application/x-tcl"           )},
    { "tex",        3, "application/x-tex"                      , sizeof("application/x-tex"           )},
    { "texi",       4, "application/x-texinfo"                  , sizeof("application/x-texinfo"       )},
    { "texinfo",    7, "application/x-texinfo"                  , sizeof("application/x-texinfo"       )},
    { "tgz",        3, "application/x-tar-gz"                   , sizeof("application/x-tar-gz"        )},
    { "tiff",       4, "image/tiff"                             , sizeof("image/tiff"                  )},
    { "tif",        3, "image/tiff"                             , sizeof("image/tiff"                  )},
    { "tsv",        3, "text/tab-separated-values"              , sizeof("text/tab-separated-values"   )},
    { "txt",        3, "text/plain"                             , sizeof("text/plain"                  )},
    { "vda",        3, "application/vda"                        , sizeof("application/vda"             )},
    { "wav",        3, "audio/x-wav"                            , sizeof("audio/x-wav"                 )},
    { "wiki",       4, "application/x-fossil-wiki"              , sizeof("application/x-fossil-wiki"   )},
    { "wma",        3, "audio/x-ms-wma"                         , sizeof("audio/x-ms-wma"              )},
    { "wmv",        3, "video/x-ms-wmv"                         , sizeof("video/x-ms-wmv"              )},
    { "wmx",        3, "video/x-ms-wmx"                         , sizeof("video/x-ms-wmx"              )},
    { "xlc",        3, "application/vnd.ms-excel"               , sizeof("application/vnd.ms-excel"    )},
    { "xll",        3, "application/vnd.ms-excel"               , sizeof("application/vnd.ms-excel"    )},
    { "xlm",        3, "application/vnd.ms-excel"               , sizeof("application/vnd.ms-excel"    )},
    { "xls",        3, "application/vnd.ms-excel"               , sizeof("application/vnd.ms-excel"    )},
    { "xlw",        3, "application/vnd.ms-excel"               , sizeof("application/vnd.ms-excel"    )},
    { "xml",        3, "text/xml"                               , sizeof("text/xml"                    )},
    { "zip",        3, "application/zip"                        , sizeof("application/zip"             )},
};

static void http_set_content_type(HttpRequest* rq, HttpResponse* res) {
    int dot;
    for(dot = rq->path.len - 1; dot > 0 && rq->path.ptr[dot] != '.'; dot--) {}
    /* i holds the index of the dot */
    const char* rq_suffix = rq->path.ptr + dot + 1;
    int rq_suffix_len = rq->path.len - dot - 1;
    for(int m = 0; m < sizeof(content_types)/sizeof(content_types[0]); m++) {
        if(rq_suffix_len == content_types[m].size && 
        strncmp(rq_suffix, content_types[m].suffix, rq_suffix_len) == 0) {
            strncpy(res->content_type, content_types[m].mime, content_types[m].mime_size);
            res->content_type[content_types[m].mime_size] = '\0';
            return;
        }
    }
    /* default to text/plain */
    strcpy(res->content_type, "text/plain");
    res->content_type[sizeof("text/plain")-1] = '\0';
}

static void http_parse(HttpRequest* rq, HttpResponse* res) {
    /*parse request*/
    rq->num_headers = 100;
    int prc;
    const char* method_str;
    size_t method_len;

    prc = phr_parse_request(rq->raw_request.ptr, rq->raw_request.len, 
                            &method_str, &method_len,
                            &rq->path.ptr, &rq->path.len, 
                            &rq->minor_version, 
                            rq->headers, &rq->num_headers, 0);
    if (prc < 0){
        log_error("HTTP parse error: %d", prc);
        res->err = HTTP_BAD_REQUEST;
    }

    /* Get the method */
    if(strncmp("GET", method_str, method_len) == 0) {
        rq->method = GET;
    } else if (strncmp("POST", method_str, method_len) == 0) {
        rq->method = POST;
    } else {
        res->err = HTTP_METHOD_NOT_ALLOWED;
    }

    http_translate_path(rq);
    http_set_content_type(rq, res);
}

static void http_validate(HttpRequest* rq, HttpResponse* res) {
    //check that rq.path is valid (ex. no '..' no leading '.' or invalid characters)
    bool is_valid = true;
    if(is_valid) {
        res->err = HTTP_OK;
    }else {
        res->err = HTTP_BAD_REQUEST;
    }
}

static const struct {
    const char* path;
    enum HttpRoute route;
} routes[] = {
    {"/perlin", ROUTE_PERLIN},
    {"/spotify-archiver", ROUTE_SPOTIFY_ARCHIVER}
};

static void http_route(HttpRequest* rq, HttpResponse* res) {
    /* translate '/' and 'index.html'*/
    if(res->err != HTTP_OK) {
        rq->route = ROUTE_ERROR;
        return;
    }

    int n_routes = sizeof(routes)/sizeof(routes[0]);
    for(int i = 0; i < n_routes; i++) {
        int route_len = sizeof(routes[i].path)-1;
        char rqp[64];
        strncpy(rqp, rq->path.ptr, rq->path.len);
        if(route_len == rq->path.len &&strncmp(rq->path.ptr, routes[i].path, route_len) == 0) {
            rq->route = routes[i].route;
            return;
        }
    }
    rq->route = ROUTE_DISK;  //default to serving file off disk
}

static void http_fill_body(HttpRequest* rq, HttpResponse* res) {
    switch(rq->route) {
        case ROUTE_DISK:
            content_read_file(rq, res);
            break;
        case ROUTE_ERROR:
            content_error(rq, res);
            break;
        case ROUTE_PERLIN:
            content_perlin(rq, res);
            break;
        case ROUTE_SPOTIFY_ARCHIVER:
            break;
        default:
            break;
    }
}

const char* http_status_code(HttpResponse* res) {
    switch(res->err) {
        case HTTP_OK:
            return "200 OK";
        case HTTP_BAD_REQUEST:
            return "400 Bad Request";
        case HTTP_FORBIDDEN:
            return "403 Forbidden";
        case HTTP_NOT_FOUND:
            return "404 Not Found";
        case HTTP_METHOD_NOT_ALLOWED:
            return "405 Method Not Allowed";
        case HTTP_SERVER_ERROR:
            return "500 Server Error";
    }
}

static void http_set_response_header(HttpResponse* res) {
    res->header.len = sprintf(res->header.ptr,
            "HTTP/1.1 %s\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: %s;\r\n"
            "Connection: close\r\n\r\n",
            http_status_code(res),
            res->body.len,
            res->content_type);
}

static void http_format_response(HttpResponse* res) {
    res->msg.msg_name = NULL,
    res->msg.msg_namelen = 0,
    res->msg.msg_control = NULL,
    res->msg.msg_controllen = 0,
    res->msg.msg_flags = 0;

    /* Generate the headers */
    http_set_response_header(res);

    /* We can use sendmsg() to send the seperate header and body buffers in a single call */
    res->iov[0].iov_base = res->header.ptr;
    res->iov[0].iov_len = res->header.len;
    res->iov[1].iov_base = res->body.ptr;
    res->iov[1].iov_len = res->body.len;

    res->msg.msg_iov = res->iov;
    res->msg.msg_iovlen = 2;
}

void http_handle_request(HttpRequest* rq, HttpResponse* res) {
    http_parse(rq, res);
    http_validate(rq, res);
    http_translate_path(rq);
    http_route(rq, res);
    http_fill_body(rq, res);
    http_format_response(res);
}