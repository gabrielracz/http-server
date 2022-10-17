#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "logger.h"

static time_t unixtime;
static struct tm * timeinfo;


void timestamp() {
	time(&unixtime);
	timeinfo = localtime(&unixtime);
	char timestr[32];
	strftime(timestr, 32, "[%T %D]", timeinfo);
	printf("%s ", timestr);
}

void log_perror(const char* msg) {
	timestamp();
	printf("ERR: %s  %s", msg, strerror(errno));
	putchar('\n');
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	timestamp();
	printf("ERR: ");
    vprintf(fmt, args);
	putchar('\n');
    va_end(args);
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	timestamp();
	printf("LOG: ");
    vprintf(fmt, args);
	putchar('\n');
    va_end(args);
}

void log_break(){
	putchar('\n');
}
