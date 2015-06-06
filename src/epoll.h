#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <sys/epoll.h>
#define MAXEVENTS 1024

int fv_epoll_create(int flags);
void fv_epoll_add(int epfd, int fs, struct epoll_event *event);
void fv_epoll_mod(int epfd, int fs, struct epoll_event *event);
void fv_epoll_del(int epfd, int fs, struct epoll_event *event);
int fv_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif
