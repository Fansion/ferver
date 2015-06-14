#include "http.h"
#include <math.h>

static int fv_http_process_ignore(fv_http_request_t *r, fv_http_out_t *out,  char *data, int len);
static int fv_http_process_connetion(fv_http_request_t *r, fv_http_out_t *out,  char *data, int len);
static int fv_http_process_if_modified_since(fv_http_request_t *r, fv_http_out_t *out,  char *data, int len);

fv_http_header_handle_t fv_http_headers_in[] = {
    {"Host", fv_http_process_ignore},
    {"Connection", fv_http_process_connetion},
    {"If-Modified-Since", fv_http_process_if_modified_since},
    {"", fv_http_process_ignore}
};

int fv_init_request_t(fv_http_request_t *r, int fd, fv_conf_t *ct)
{
    r->fd = fd;
    r->pos = r->last = r->buf;
    r->state = 0;
    r->root = ct->root;
    INIT_LIST_HEAD(&r->list);
    return FV_OK;
}

int fv_free_request_t(fv_http_request_t *r)
{
    //TODO
    return FV_OK;
}

int fv_init_out_t(fv_http_out_t *o, int fd)
{
    o->fd = fd;
    o->keep_alive = 0;
    o->modified = 1;
    o->status = 0;
    return FV_OK;
}

int fv_free_out_t(fv_http_out_t *o)
{
    //TODO
    return FV_OK;
}

void fv_http_handle_header(fv_http_request_t *r, fv_http_out_t *o)
{
    list_head *pos;
    fv_http_header_t *hd;
    fv_http_header_handle_t *header_in;

    // log_info("## header　begin");
    list_for_each(pos, &r->list) {
        hd = list_entry(pos, fv_http_header_t, list);
        // handle
        // log_info("### %.*s:%.*s",
        //          (int)(hd->key_end - hd->key_start),
        //          (char *)hd->key_start,
        //          (int)(hd->value_end - hd->value_start),
        //          (char *)hd->value_start);
        for (header_in = fv_http_headers_in;
                strlen(header_in->name) > 0;
                header_in++) {
            if (strncmp(hd->key_start, header_in->name, hd->key_end - hd->key_start) == 0)
            {
                log_info("find match: %s", header_in->name);
                (*(header_in->handler))(r, o, hd->value_start, hd->value_end - hd->value_start);
            }
        }
        // delete it from original list
        list_del(pos);
        free(hd);
    }
    // log_info("## header　end");
}

int fv_http_process_ignore(fv_http_request_t *r, fv_http_out_t *out,  char *data, int len)
{
    return FV_OK;
}

int fv_http_process_connetion(fv_http_request_t *r, fv_http_out_t *out, char *data, int len)
{
    if (strncasecmp("keep-alive", data, len) == 0)
    {
        out->keep_alive = 1;
    }
    return FV_OK;
}

int fv_http_process_if_modified_since(fv_http_request_t *r, fv_http_out_t *out, char *data, int len)
{
    struct tm tm;
    strptime(data, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    time_t client_time = mktime(&tm);

    double time_diff = difftime(out->mtime, client_time);
    if (fabs(time_diff) == 0)
    {
        debug("file not modified since last request!");
        out->modified = 0;
        out->status = FV_HTTP_NOT_MODIFIED;
    }
    return FV_OK;
}

const char *get_shortmsg_from_status_code(int status_code)
{
    if (status_code == FV_HTTP_OK)
    {
        return "OK";
    }
    if (status_code == FV_HTTP_NOT_MODIFIED)
    {
        return "NOT MODIFIED";
    }
    if (status_code == FV_HTTP_NOT_FOUND)
    {
        return "NOT FOUND";
    }
    return "UNKNOWN";
}
