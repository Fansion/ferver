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
#include <stdlib.h>

#include "dbg.h"

// max number of listen queue
#define LISTENQ         1024
#define SERVER_PORT     3000
#define BUFFLEN         8192
#define DELIM            "="
#define FV_CONF_OK         0
#define FV_CONF_ERROR    100

typedef struct fv_conf_s {
    void *root;
    int port;
    int thread_num;
} fv_conf_t;


int open_listenfd(int port);
int make_socket_non_blocking(int fd);
int read_conf(char *filename, fv_conf_t *cf, char *buf, int len);


#endif
