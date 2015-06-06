#ifndef __HTTP_H__
#define __HTTP_H__

#include <strings.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "rio.h"

typedef struct mime_type_s
{
    const char *type;
    const char *value;
} mime_type_t;

void read_requesthdrs(rio_t *rio);
void parse_uri(char *uri, char *filename, char *querystring);
const char* get_file_type(const char *type);
void serve_static(int fd, char *filename, int filesize);

void do_request(int fd);
void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

#endif
