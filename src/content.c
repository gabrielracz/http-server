#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "http.h"
#include "logger.h"
#include "content.h"
#include "perlin.h"
#include "util.h"


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
    strcpy(res->body.ptr + res->body.len, "</pre></body>");
    res->body.len += 14;
    strcpy(res->content_type, "text/html");
}

void content_read_file(HttpRequest* rq, HttpResponse* res)
{
    FILE* fp;
    char path[1024] = "resources/";
    strncat(path, rq->path.ptr, rq->path.len);

    fp = fopen(path, "r");
    if (fp == NULL) {
        // perror("http_read_file, could not open requested file");
        res->err = HTTP_NOT_FOUND;
        content_error(rq, res);
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t filelen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filelen > res->body.size) {
        //Transfer encoding chunked support goes here
        res->err = HTTP_CONTENT_TOO_LARGE;
        fclose(fp);
        return;
    }

    size_t bytes_read;
    bytes_read = fread(res->body.ptr, 1, filelen, fp);
    res->body.ptr[bytes_read] = '\0';
    res->body.len = bytes_read;
    rq->done = true;
    fclose(fp);
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
}

void content_perlin(HttpRequest* rq, HttpResponse* res) {
    content_begin_plaintext_wrap(res);
    //make it html
	int w = 250;
	int h = 100;
	size_t n = PGRIDSIZE(w,h);
    if(n > res->body.size - res->body.len) {
        res->err = HTTP_BAD_REQUEST;
        content_error(rq, res);
        return;
    }
	res->body.len += perlin_sample_grid(res->body.ptr + res->body.len, res->body.size-res->body.len, w, h, 
						randf(200.0f), randf(200.0f), randf(0.1f));
    content_end_plaintext_wrap(res);
}

enum VariableType {
    INT,
    FLOAT,
    BOOL,
    STRING
};

#define VARIABLE_LEN 128
void content_archiver(HttpRequest* rq, HttpResponse* res) {

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
        content_error(rq, res);
        return;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        res->err = HTTP_SERVER_ERROR;
        content_error(rq, res);
        return;
    } else if (pid == 0) {
        // Child process
        close(fd[0]); // Close the read end of the pipe

        // Redirect standard output to the write end of the pipe
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            res->err = HTTP_SERVER_ERROR;
            content_error(rq, res);
            return;
        }
        if (dup2(fd[1], STDERR_FILENO) == -1) {
            perror("dup2");
            res->err = HTTP_SERVER_ERROR;
            content_error(rq, res);
            return;
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
            res->err = HTTP_SERVER_ERROR;
            content_error(rq, res);
        }
        close(fd[1]);
    } else {
        // Parent process
        close(fd[1]); // Close the write end of the pipe

        // Read from the pipe until the end of file
        while (( bytes_read = read(fd[0], res->body.ptr + res->body.len, res->body.size - res->body.len)) != 0) {
            if (bytes_read == -1) {
                perror("read");
                res->err = HTTP_SERVER_ERROR;
                content_error(rq, res);
                return;
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
    }
}
