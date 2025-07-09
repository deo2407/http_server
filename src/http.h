#ifndef HTTP_H
#define HTTP_H

void http_handle_request(int sender_fd,const char *buf, int size);

#endif
