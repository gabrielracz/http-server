#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "logger.h"

#if LOGGING == 1

void unix_timestamp() {
	time_t unixtime;
	struct tm* timeinfo;

	time(&unixtime);
	timeinfo = localtime(&unixtime);
	char timestr[32];
	strftime(timestr, 32, "[%T %D]", timeinfo);
	printf("%s ", timestr);
}

void timestamp() {
	struct timeval tv;
	struct tm timeinfo;
	gettimeofday(&tv, NULL);
	if((localtime_r(&tv.tv_sec, &timeinfo))){
		char fmt[64];
		char timestr[64];
		// strftime(fmt, 64, "[%d/%b/%y %H:%M:%S.%%03lu]", &timeinfo);
        // snprintf(timestr, sizeof(timestr), fmt, tv.tv_usec/1000);
		strftime(fmt, 64, "[%d/%b/%y %H:%M:%S]", &timeinfo);
        strcpy(timestr, fmt);
		printf("%s ", timestr);
	}
}

void log_perror(const char* msg) {
	timestamp();
	printf("ERR: %s  %s", msg, strerror(errno));
	putchar('\n');
	fflush(stdout);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	timestamp();
	printf("ERR: ");
    vprintf(fmt, args);
	putchar('\n');
    va_end(args);
	fflush(stdout);
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
	timestamp();
	// printf("LOG: ");
    vprintf(fmt, args);
	putchar('\n');
    va_end(args);
	fflush(stdout);
}

void log_break(){
	putchar('\n');
	fflush(stdout);
}

#else

void log_init();
void log_perror(const char* msg){}
void log_error(const char* fmt, ...){}
void log_info(const char* fmt, ...){}
void log_break(){}

#endif
