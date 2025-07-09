#include "server.h"
#include "logger.h"

int main(void) {
    if (logger_init("server.log") != 0) {
        fprintf(stderr, "Logger init failed\n");
        return 1;
    }

    int listener = server_init();
    if (listener == -1) {
        logger_close();
        return 1;
    }

    server_run(listener);
    server_close(listener);
    logger_close();

    return 0;
}

