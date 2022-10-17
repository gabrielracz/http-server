#ifndef LOG_H
#define LOG_H

//typedef struct {
	//char timestamp_buf[32];
//} Logger;

void log_init();
void log_perror(const char* msg);
void log_error(const char* fmt, ...);
void log_info(const char* fmt, ...);

#endif
