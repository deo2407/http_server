#include "http.h"
#include "logger.h"
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#define RESPONSE_BUF_SIZE 8192

#define MAX_HEADERS 32
#define MAX_HEADER_NAME_LEN 64
#define MAX_HEADER_VALUE_LEN 256

typedef struct {
    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
} header_t;

typedef struct {
    char method[5];
    char path[128];
    char http_version[16];

    // headers struct
    header_t headers[MAX_HEADERS];
    int header_count;

    char *body;
    size_t body_lenght;
} request_t;

typedef struct {
    char http_version[16];
    int status_code;      // e.g. 200, 400, 404
    char status_text[64]; // e.g. "OK", "FOUND", "Bad Request"

    char content_type[64];
    char content_length[16];

    char *body;
    long body_length;
} response_t;

int send_all(int fd, const char *buf, size_t len) {
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t sent = send(fd, buf + total_sent, len - total_sent, 0);

        if (sent <= 0) {
            logger_perror("send");
            return -1;
        }
        total_sent += sent;
    }
    return 0;
}

int http_parse_request(const char *request_buf, int size, 
    request_t *req) {

    // Parse first line
    const char *pos = strstr(request_buf, "\r\n");
    if (!pos) return -1; // bad format
    size_t line_len = pos - request_buf;

    char request_line[512];
    if (line_len >= sizeof(request_line)) return -1;
    memcpy(request_line, request_buf, line_len);
    request_line[line_len] = '\0';
    
    // Tokenize request line
    // Check method
    char *method = strtok(request_line, " ");
    if (!(strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 ||
            strcmp(method, "HEAD") == 0)) {
        return -1;
    }
    strncpy(req->method, method, sizeof(req->method) - 1);

    // Check path
    char *path = strtok(NULL, " ");
    if (!path) 
        return -1;
    strncpy(req->path, path, sizeof(req->path) - 1);
    
    // Check version
    char *version = strtok(NULL, " ");
    // Version checking needs to be improved
    if (!(strcmp(version, "HTTP/") == 0)) {
        return -1;
    }
    strncpy(req->http_version, version, sizeof(req->http_version) - 1);
    
    // Headers parsing must be added
    
    return 0;
}

void bad_response(response_t *response) {
    response->status_code = 404;
    strcpy(response->status_text, "Not Found");
}

int generate_content(const char *filename, response_t *response) {
    FILE *file = NULL;
    char *body;

    // Generate the path
    char filepath[64];
    sprintf(filepath, "../public/%s", filename);

    file = fopen(filepath, "rb");
    if (!file) {
        logger_perror("fopen");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    if (file_size < 0) {
        fclose(file);
        return -1;
    }

    body = (char *)malloc(file_size);
    if (!body) {
        fclose(file);
        logger_perror("malloc");
        return -1;
    }

    logger_log(LOG_INFO, "adding file %s to the response", filename);

    strcpy(response->content_type, "text/html");
    
    char size_str[10];
    sprintf(size_str, "%ld", file_size);
    strcpy(response->content_length, size_str);

    response->body = body;
    response->body_length = file_size;
    
    return 0;
}

void generate_get_response(response_t *response, request_t *request) {
    strcpy(response->http_version, "HTTP/1.0");

    if (strcmp(request->path, "/") == 0) {
        response->status_code = 200;
        strcpy(response->status_text, "OK");

        return;
    } 

    int res = generate_content(request->path, response);
    if (res < 0) {
        bad_response(response);
    }

    response->status_code = 200;
    strcpy(response->status_text, "OK");
}

void generate_response(response_t *response, request_t *request) {
    if (strcmp(request->method, "GET") == 0) {
        generate_get_response(response, request);
    }
}

int send_response(int sender_fd, response_t *response) {
    char response_str[4096];
    int len = snprintf(response_str, sizeof(response_str),
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %s\r\n"
        "Content-Type: %s\r\n"
        "\r\n"
        "%s", 
        response->status_code,
        response->status_text,
        response->content_length,
        response->content_type,
        response->body);

    send_all(sender_fd, response_str, len);
    return 0;
}

void http_handle_request(int sender_fd, const char *request_buf, int size) {
    request_t *request = (request_t *)malloc(sizeof(request_t));
    response_t *response = (response_t *)malloc(sizeof(response_t));
    int valid;
    
    if ((valid = http_parse_request(request_buf, size, request)) == -1) {
        response->status_code = 400;
        strcpy(response->status_text, "Bad Request");
        
        logger_log(LOG_INFO, "sending response with status code 400: Bad Request");
        
        send_response(sender_fd, response);
        return;
    }

    generate_response(response, request);

    logger_log(LOG_INFO, "sending response with status code %d: %s",
            response->status_code, response->status_text);
    send_response(sender_fd, response);
}

