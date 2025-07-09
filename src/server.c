#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#include "logger.h"
#include "http.h"

#define PORT "4000"   // Port we're listening on

const char *inet_ntop2(void *addr, char *buf, size_t len) {
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
        case AF_INET:
            sa4 = addr;
            src = &(sa4->sin_addr);
            break;
        case AF_INET6:
            sa6 = addr;
            src = &(sa6->sin6_addr);
            break;
        default:
            return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, len);
}

int server_get_listener(void) {
    struct addrinfo hints, *ai, *p;
    int listener;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        logger_log(LOG_ERROR, "http_server: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol);
        
        if (listener < 0) 
            continue;

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL) {
        return -1;
    }

    freeaddrinfo(ai);

    if (listen(listener, 20) == -1) {
        logger_perror("listen");
        return -1;
    }

    return listener;
}

void add_to_pfds(int newfd, int *fd_count, int *fd_size,
        struct pollfd **pfds) {
    if (*fd_count == *fd_size) {
        *fd_size *= 2;
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));

        if (!pfds) {
            logger_log(LOG_ERROR, "realloc failed");
            exit(1);
        }
    }


    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;
    (*pfds)[*fd_count].revents = 0;

    (*fd_count)++;
}

void remove_from_pfds(int i, struct pollfd pfds[], int *fd_count) {
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

void server_handle_client(int *fd_count,
        struct pollfd *pfds, int *pfd_i) {
    
    char buf[1024];

    int nbytes = recv(pfds[*pfd_i].fd, buf, sizeof buf, 0);

    int sender_fd = pfds[*pfd_i].fd;

    if (nbytes <= 0) { // Got error or connection closed by client
        if (nbytes == 0) {
            logger_log(LOG_INFO, "socket %d hung up", sender_fd);
        } else {
            logger_perror("recv");
        }

        close(pfds[*pfd_i].fd);

        remove_from_pfds(*pfd_i, pfds, fd_count);

        // reexamine the slot we just deleted
        (*pfd_i)--;

    } else {  
        buf[nbytes] = '\0';
        logger_log(LOG_INFO, "recv from fd %d: %s", sender_fd, buf);

        http_handle_request();
    }
}

void server_accept_connection(int listener, int *fd_count, int *fd_size,
        struct pollfd **pfds) {
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    int newfd;
    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;
    newfd = accept(listener, (struct sockaddr*)&remoteaddr,
        &addrlen);

    if (newfd == -1) {
        logger_perror("accept");
    } else {
        add_to_pfds(newfd, fd_count, fd_size, pfds);

        logger_log(LOG_INFO, "new connection from %s on socket %d",
            inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
            newfd);
    }
}

void server_process_connections(int listener, int *fd_count, int *fd_size,
        struct pollfd **pfds) {
    for (int i = 0; i < *fd_count; i++) {

        if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
            
            if ((*pfds)[i].fd == listener) {
                server_accept_connection(listener, fd_count, fd_size, 
                        pfds);
            } else {
                server_handle_client(fd_count, *pfds, &i);
            }
        }
    }
}

int server_init(void) {
    int listener = server_get_listener();

    if (listener == -1) {
        logger_log(LOG_ERROR, "error getting listening socket");
        return -1;
    }

    return listener;
}

void server_run(int listener) {
    int fd_size = 5;
    int fd_count = 0;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);
        
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;
    fd_count = 1;

    logger_log(LOG_INFO, "server started on socket %d", listener);
    logger_log(LOG_INFO, "waiting for connections...");

    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1) {
            logger_perror("poll");
            break;
        }

        server_process_connections(listener, &fd_count, &fd_size,
                &pfds);
    }

    free(pfds);
}

void server_close(int listener) {
    close(listener);
    logger_log(LOG_INFO, "server socket %d closed", listener);
}

