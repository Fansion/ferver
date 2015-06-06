#ifndef __HTTP_H__
#define __HTTP_H__

#include <strings.h>
#include <stdio.h>

void do_request(int fd);
void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

#endif
