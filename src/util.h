#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include "dbg.h"

// max number of listen queue
#define LISTENQ 1024
#define SERVER_PORT 3000


int open_listenfd(int port);
int make_socket_non_blocking(int fd);


#endif
