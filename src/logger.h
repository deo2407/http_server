#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <string.h>

typedef enum {
    LOG_INFO,
    LOG_ERROR
} LogLevel;

int logger_init(const char *filename);

// Log a message with a format string (like printf)
void logger_log(LogLevel level, const char *format, ...);

void logger_perror(const char *msg);

void logger_close();

#endif 

