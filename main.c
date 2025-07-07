#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "4000"   // Port we're listening on

// Plan:
// Create a socket
// Bind the socket to an address
// Listen on the address
// Block on Accept until a connection is made
// Read the connection socket
// Figuire out how to respond
// Write back to the socket
// Close the connection
// Go back to blocking on accept

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

int get_listener_socket(void) {
    struct addrinfo hints, *ai, *p;
    int listener;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "http_server: %s\n", gai_strerror(rv));
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
        return -1;
    }

    return listener;
}

void respond(int newfd) {
    const char *response = "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 14\r\n"
        "\r\n"
        "Hello, world!\n";

    if (send(newfd, response, strlen(response), 0) == -1) {
        perror("send");
    } 
}

void process_connections(int listener) {
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    int newfd;
    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;
    newfd = accept(listener, (struct sockaddr*)&remoteaddr,
        &addrlen);

    if (newfd == -1) {
        perror("accept");
    } else {
        printf("new connection from %s on socket %d\n",
            inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
            newfd);
        respond(newfd);
    }
    close(newfd);
}

int main(void) {
    int listener;
        
    listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    for(;;) {
        process_connections(listener);
    }

    return 0;
}
