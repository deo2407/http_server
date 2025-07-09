#ifndef SERVER_H
#define SERVER_H

int server_init(void);
void server_run(int listener);
void server_close(int listener);

#endif
