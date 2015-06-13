#ifndef __HTTP_H__
#define __HTTP_H__

#include <strings.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "rio.h"
#include "list.h"

#define FV_AGAIN                    EAGAIN
#define FV_OK                            0
#define FV_HTTP_PARSE_INVALID_METHOD    10
#define FV_HTTP_PARSE_INVALID_REQUEST   11
#define FV_HTTP_PARSE_INVALID_HEADER    12

#define FV_HTTP_UNKNOWN             0x0001
#define FV_HTTP_GET                 0x0002
#define FV_HTTP_HEAD                0x0004
#define FV_HTTP_POST                0x0008

#define MAX_BUF                       8124

typedef struct mime_type_s
{
    const char *type;
    const char *value;
} mime_type_t;

typedef struct fv_http_request_s {
    int fd;
    char buf[MAX_BUF];
    void *pos, *last;           // change from int pos, last;
    int state;
    void *request_start;
    void *method_end;           // method_end not included
    int method;
    void *uri_start;
    void *uri_end;              // uri_end not included
    int http_major;
    int http_minor;
    void *request_end;

    list_head list;             // store http header
    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;
} fv_http_request_t;

typedef struct fv_http_header_s {
    void *key_start, *key_end;
    void *value_start, *value_end;
    list_head list;
} fv_http_header_t;

void read_requesthdrs(rio_t *rio);
void parse_uri(char *uri, int uri_length, char *filename, char *querystring);
const char* get_file_type(const char *type);
void serve_static(int fd, char *filename, int filesize);

void do_request(void *infd);
void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int fv_http_parse_request_line(fv_http_request_t *r);
int fv_http_parse_request_body(fv_http_request_t *r);
int fv_init_request_t(fv_http_request_t *r, int fd);
void fv_http_handle_header(fv_http_request_t *r);

#endif
