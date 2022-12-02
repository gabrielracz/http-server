#ifndef LOG_H
#define LOG_H

#define LOGGING 1

void log_init();
void log_perror(const char* msg);
void log_error(const char* fmt, ...);
void log_info(const char* fmt, ...);
void log_break();

#endif
