#include "logger.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int logger_init(const char *filename) {
    log_file = fopen(filename, "a");
    if (!log_file) return -1;

    return 0;
}

void logger_log(LogLevel level, const char *format, ...) {
    if (!log_file) return;

    pthread_mutex_lock(&log_mutex);
    
    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof time_str, "%Y-%m-%d %H:%M:%S", t);

    const char *level_str = (level == LOG_INFO) ? "INFO" : "ERROR";
    
    char log_buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer, sizeof log_buffer, format, args);
    va_end(args);

    // Write to log file
    fprintf(log_file, "[%s] %s: %s\n", time_str, level_str, log_buffer);
    fflush(log_file);

#ifdef LOGGER_ENABLE_CONSOLE
    fprintf(stderr, "[%s] %s: %s\n", time_str, level_str, log_buffer);
    fflush(stderr);
#endif

    pthread_mutex_unlock(&log_mutex);
}

void logger_perror(const char *msg) {
    logger_log(LOG_ERROR, "%s: %s", msg, strerror(errno));
}

void logger_close() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
