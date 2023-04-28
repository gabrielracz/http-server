#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "http.h"
#include "logger.h"
#include "content.h"
#include "perlin.h"
#include "util.h"
#include "libsha.h"

void content_init() {
    perlinit(1337);
}

const char html_plaintext_wrapper[] =
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<title>gsracz</title>"
    "<link rel=\"stylesheet\" href=\"/style.css\">"
    "</head>"
    "<body>\n"
    "<pre class=\"plaintext\">";
const size_t html_plaintext_wrapper_size = sizeof(html_plaintext_wrapper)-1;

static void content_begin_plaintext_wrap(HttpResponse* res) {
    strncpy(res->body.ptr, html_plaintext_wrapper, html_plaintext_wrapper_size);
    res->body.len = html_plaintext_wrapper_size;
}

static void content_end_plaintext_wrap(HttpResponse* res) {
    const char footer[] = "</pre></body>";
    strcpy(res->body.ptr + res->body.len, footer);
    res->body.len += sizeof(footer);
    strcpy(res->content_type, "text/html");
}

size_t content_read_file(HttpRequest* rq, HttpResponse* res)
{
    FILE* fp;
    char filename[1024] = "resources/";
    strncat(filename, rq->path.ptr, rq->path.len);

    struct stat fstat;
    if(stat(filename, &fstat) == -1) {
        res->err = HTTP_SERVER_ERROR;
        log_perror("stat");
        return content_error(rq, res);
    }

    if(!S_ISREG(fstat.st_mode)) { // we don't allow directory listing atm.
        res->err = HTTP_FORBIDDEN;
        return content_error(rq, res);
    }

    size_t file_length = fstat.st_size;
    rq->target_output_length = file_length;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        res->err = HTTP_NOT_FOUND;
        log_perror("fopen");
        return content_error(rq, res);
    }


    if (file_length > res->body.size) {
        // if(file_length > )
        //
        //Transfer encoding chunked support goes here
        res->err = HTTP_CONTENT_TOO_LARGE;
        fclose(fp);
        return content_error(rq, res);
    }

    size_t bytes_read;
    bytes_read = fread(res->body.ptr, 1, file_length, fp);

    res->body.ptr[bytes_read] = '\0';
    res->body.len = bytes_read;
    fclose(fp);
    return bytes_read;
}

size_t content_error(HttpRequest* rq, HttpResponse* res) {
    const char* status_code_str = http_status_code(res);
    res->body.len = sprintf(res->body.ptr,
           "<!DOCTYPE html>"
           "<html>"
           "<head>"
                "<meta charset=\"UTF-8\">"
                "<title>%s</title>"
                "<link rel=\"stylesheet\" href=\"/style.css\">"
           "</head>"
           "<body>"
           "<center><h1>%s</h1></center>"
           "<hr><center>naoko/2.0</center>"
           "</body>"
           "</html>",
           status_code_str,
           status_code_str);
    strcpy(res->content_type, "text/html");
    return res->body.len;
}

size_t content_perlin(HttpRequest* rq, HttpResponse* res) {
    content_begin_plaintext_wrap(res);
    //make it html
	int w = 250;
	int h = 100;
	size_t n = PGRIDSIZE(w,h);
    if(n > res->body.size - res->body.len) {
        res->err = HTTP_BAD_REQUEST;
        return content_error(rq, res);
    }
	res->body.len += perlin_sample_grid(res->body.ptr + res->body.len, res->body.size-res->body.len, w, h, 
						randf(200.0f), randf(200.0f), randf(0.1f));
    content_end_plaintext_wrap(res);
    return res->body.len;
}

enum VariableType {
    INT,
    FLOAT,
    BOOL,
    STRING
};

#define VARIABLE_LEN 128
size_t content_archiver(HttpRequest* rq, HttpResponse* res) {

    content_begin_plaintext_wrap(res);
    int fd[2];
    pid_t pid;
    int bytes_read;

    // char username[VARIABLE_LEN] = "";
    char* username = "";
    // char playlist_name[VARIABLE_LEN] = "";
    char* playlist_name = "";
    bool show_albums = false;
    bool list_playlists = false;

    for(int i = 0; i < rq->n_variables; i++) {
		// printf("%.*s : %.*s\n", rq->variables[i].key.len, rq->variables[i].key.ptr, rq->variables[i].value.len, rq->variables[i].value.ptr);

        int len = min(rq->variables[i].value.len, VARIABLE_LEN);
        //if(rq->variables[i].value.len == 0) {continue;}
        if(strncmp("username", rq->variables[i].key.ptr, rq->variables[i].key.len) == 0) {
            username = rq->variables[i].value.ptr;
        } else if (strncmp("playlist_name", rq->variables[i].key.ptr, rq->variables[i].key.len) == 0) {
            playlist_name = rq->variables[i].value.ptr;
        } else if (strncmp("list_playlists", rq->variables[i].key.ptr, rq->variables[i].key.len) == 0) {
            list_playlists = true;
        } else if (strncmp("show_albums", rq->variables[i].key.ptr, rq->variables[i].key.len) == 0) {
            show_albums = true;
        } 
    }

    if (pipe(fd) == -1) {
        perror("pipe");
        res->err = HTTP_SERVER_ERROR;
        return content_error(rq, res);
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        res->err = HTTP_SERVER_ERROR;
        return content_error(rq, res);
    } 
    
    if (pid == 0) {
        // Child process
        close(fd[0]); // Close the read end of the pipe

        // Redirect standard output to the write end of the pipe
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(-1);
        }
        if (dup2(fd[1], STDERR_FILENO) == -1) {
            perror("dup2");
            exit(-1);
        }

        // Execute the
        char* args[7] = { NULL };
        args[0] = "node";
        args[1] = "tools/spotify-archiver/spotify-archiver.js";
        args[2] = username;

        if(list_playlists) {
            args[3] = "--list";
        } else {
            args[3] = playlist_name;
            if(show_albums) {
                args[4] = "--albums";
            }
        }

        if (execvp("node", args) == -1) {
            perror("execlp");
            exit(-1);
        }
        close(fd[1]);
        exit(0);
    } else {
        // Parent process
        close(fd[1]); // Close the write end of the pipe

        // Read from the pipe until the end of file
        while (( bytes_read = read(fd[0], res->body.ptr + res->body.len, res->body.size - res->body.len)) != 0) {
            if (bytes_read == -1) {
                perror("read");
                res->err = HTTP_SERVER_ERROR;
                return content_error(rq, res);
            }
            res->body.len += bytes_read;
        }

        // Wait for the child process to exit and collect its exit status
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            res->err = HTTP_SERVER_ERROR;
            content_error(rq, res);
        } else if (WEXITSTATUS(status)) {
            res->err = HTTP_SERVER_ERROR;
            content_error(rq, res);
        } else {
            content_end_plaintext_wrap(res);
        }
        close(fd[0]);
        return res->body.len;
    }
    return 0;
}

size_t content_sha256(HttpRequest* rq, HttpResponse* res) {
    StringView input = {.ptr = NULL};
    for(int i = 0; i < rq->n_variables; i++) {
        if(strncmp("sha_input", rq->variables[i].key.ptr, rq->variables[i].key.len) == 0) {
            input.ptr = rq->variables[i].value.ptr;
            input.len = rq->variables[i].value.len;
        }
    }
    if(!input.ptr) {
        res->err = HTTP_BAD_REQUEST;
        return content_error(rq, res);
    }

    // content_begin_plaintext_wrap(res);
    size_t bytes = sha256(input.ptr, input.len, res->body.ptr + res->body.len);
    res->body.len += bytes;
    // content_end_plaintext_wrap(res);
    return res->body.len;
}