#include "http.h"
#include "logger.h"
#include <sys/socket.h>
#include <string.h>

#define RESPONSE_BUF_SIZE 8192

int sendall(int fd, const char *buf, size_t len) {
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t sent = send(fd, buf, len, 0);

        if (send <= 0) {
            logger_perror("send");
            return -1;
        }
        total_sent += sent;
    }
    return 0;
}

int generate_response(const char *request, int size,
        char *buf, size_t buf_size) {
    // TODO: parse request and generate response
    // for now send a fixed response

    const char *resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, world!";
    
    if (strlen(resp) >= buf_size)
        return -1;

    strcpy(buf, resp);
    return strlen(resp);
}

void http_handle_request(int sender_fd, const char *request, 
        int size) {

    char response[RESPONSE_BUF_SIZE];
    int resp_len = generate_response(request, size, 
            response, sizeof response);

    sendall(sender_fd, response, strlen(response));
}

