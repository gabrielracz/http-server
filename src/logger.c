#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include "logger.h"

static time_t unixtime;
static struct tm * timeinfo;

static const char* timestamp() {
	time(&unixtime);
	timeinfo = localtime(&unixtime);
	//potentially need to clear this
	static char buf[32];
	strftime(buf, 32, "%T %D ", timeinfo);
	return buf;
}

void log_perror(const char* msg) {
	puts(timestamp());
	perror(msg);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	puts(timestamp());
    vprintf(fmt, args);
    va_end(args);
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	puts(timestamp());
    vprintf(fmt, args);
    va_end(args);
}
