#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "http.h"
#include <time.h>

#define FV_AGAIN                    EAGAIN
#define FV_OK                            0
#define FV_HTTP_PARSE_INVALID_METHOD    10
#define FV_HTTP_PARSE_INVALID_REQUEST   11
#define FV_HTTP_PARSE_INVALID_HEADER    12

#define FV_HTTP_UNKNOWN             0x0001
#define FV_HTTP_GET                 0x0002
#define FV_HTTP_HEAD                0x0004
#define FV_HTTP_POST                0x0008

#define FV_HTTP_OK                     200
#define FV_HTTP_NOT_MODIFIED           304
#define FV_HTTP_NOT_FOUND              404

#define MAX_BUF                       8124

typedef struct fv_http_request_s {
    void *root;
    int fd;
    char buf[MAX_BUF];
    void *pos, *last;           // change from int pos, last;
    int state;
    void *request_start;
    void *method_end;           // method_end not included
    int method;
    void *uri_start;
    void *uri_end;              // uri_end not included
    void *path_start;
    void *path_end;
    void *query_start;
    void *query_end;
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

typedef struct {
    int fd;
    int keep_alive;
    time_t mtime;
    int modified;
    int status;
} fv_http_out_t;

typedef int (*fv_http_header_handler_pt) (fv_http_request_t *r, fv_http_out_t *o, char *data, int len);
typedef struct {
    char *name;                             // 定义header头名称
    fv_http_header_handler_pt handler;      // 定义header头对应的解析函数
} fv_http_header_handle_t;

void fv_http_handle_header(fv_http_request_t *r, fv_http_out_t *o);

int fv_init_request_t(fv_http_request_t *r, int fd, fv_conf_t *ct);
int fv_free_request_t(fv_http_request_t *r);

int fv_init_out_t(fv_http_out_t *o, int fd);
int fv_free_out_t(fv_http_out_t *o);

const char *get_shortmsg_from_status_code(int status_code);

#endif
