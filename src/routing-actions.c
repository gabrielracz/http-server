#include <stdio.h>
#include "logger.h"

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
	log_info("file read complete");
    return filelen;
}