#include "http.h"
#include <strings.h>

static void parse_uri(char *uri, int length, char *filename, char *querystring);
static const char *get_file_type(const char *type);
static void serve_static(int fd, char *filename, int filesize, fv_http_out_t *out);
static void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

mime_type_t ferver_mime[] =
{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {NULL , "text/plain"}
};

void parse_uri(char *uri, int uri_length,  char *filename, char *querystring)
{
    char *question_mark = index(uri, '?');
    int file_length;
    if (question_mark) {
        file_length = (int)(question_mark -uri);
    } else {
        file_length = uri_length;
    }

    strcpy(filename, ROOT);
    strncat(filename, uri, file_length);

    char *last_dot = rindex(filename, '.');

    if (last_dot == NULL && filename[strlen(filename) - 1] != '/') {
        strcat(filename, "/");
    }
    if (filename[strlen(filename) - 1] == '/') {
        strcat(filename, "index.html");
    }
}

const char* get_file_type(const char *type)
{
    if (type == NULL)
        return "text/plain";
    int i;
    for (i = 0; ferver_mime[i].type != NULL; ++i)
    {
        if (strcmp(type, ferver_mime[i].type) == 0)
            return ferver_mime[i].value;
    }
    return ferver_mime[i].value;
}

void serve_static(int fd, char *filename, int filesize, fv_http_out_t *out)
{
    log_info("## ready to serve_static");
    log_info("filename = %s", filename);
    char header[MAXLINE];
    char buf[SHORTLINE];
    int n;
    struct tm tm;

    const char *file_type;
    const char *dot_pos = rindex(filename, '.');
    file_type = get_file_type(dot_pos);

    sprintf(header, "HTTP/1.1 %d %s\r\n", out->status, get_shortmsg_from_status_code(out->status));

    if (out->keep_alive)
    {
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    }
    if (out->modified)
    {
        sprintf(header, "%sContent-type: %s\r\n", header, file_type);
        sprintf(header, "%sContent-length: %d\r\n", header, filesize);
        localtime_r(&(out->mtime), &tm);
        strftime(buf, SHORTLINE,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buf);
    }
    sprintf(header, "%sServer: Ferver\r\n", header);
    sprintf(header, "%s\r\n", header);

    n = rio_writen(fd, header, strlen(header));
    check(n == (ssize_t)strlen(header), "rio_writen error");
    if (!out->modified) {
        return;
    }
    // use sendfile
    int srcfd = open(filename, O_RDONLY, 0);
    check(srcfd > 2, "open error");
    char *srcaddr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr != 0, "mmap error");
    close(srcfd);

    n = rio_writen(fd, srcaddr, filesize);
    check(n == filesize, "rio_writen error");

    munmap(srcaddr, filesize);
    log_info("## serve_static suc");
}

void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char header[MAXLINE], body[MAXLINE];
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n</p>", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny web server</em>\r\n", body);

    sprintf(header, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(header, "%sServer: ferver\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));
    log_info("err response header  = \n %s", header);
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    log_info("leave do_error");
}

void do_request(void *ptr)
{
    fv_http_request_t *r = (fv_http_request_t *)ptr;
    int fd = r->fd;
    int rc;
    char filename[SHORTLINE];
    struct stat sbuf;
    int n;

    for (;;)
    {
        log_info("# ready to serve client(fd %d)", fd);
        n = read(fd, r->last, (uint32_t)r->buf + MAX_BUF - (uint32_t)r->last);  // GET / HTTP/1.1 16bytes
        log_info("buffer remaining: %d", (uint32_t)r->buf + MAX_BUF - (uint32_t)r->last);
        // log_info("n = %d, errno = %d", n, errno);
        if (n == 0) {   // EOF
            log_info("read return 0, ready to close fd %d", fd);
            goto err;
        }
        if (n < 0) {
            if (errno != EAGAIN) {
                log_err("read err");
                goto err;
            }
            break;      // errno == EAGAIN
        }
        r->last += n;
        check((uint32_t)r->last <= (uint32_t)r->buf + MAX_BUF, "r->last shoule <= MAX_BUF");

        log_info("## ready to parse request line");
        log_info("enter fv_http_parse_request_line, start addr=%d, last addr=%d, head length=%d", (int)r->pos, (int)r->last, (int)(r->last - r->pos));
        rc = fv_http_parse_request_line(r);
        if (rc == FV_AGAIN) {
            continue;
        } else if (rc != FV_OK) {
            log_err("rc != FV_OK");
            goto err;
        }
        log_info("request method: %.*s", (int)(r->method_end - r->request_start), (char *)r->request_start);
        log_info("request uri: %.*s", (int)(r->uri_end - r->uri_start), (char *)r->uri_start);
        log_info("## parse request line suc");


        log_info("## ready to parse request body");
        log_info("enter fv_http_parse_request_body,  start addr=%d, last addr=%d, body length=%d", (int)r->pos, (int)r->last, (int)(r->last - r->pos));
        rc = fv_http_parse_request_body(r);
        if (rc == FV_AGAIN) {
            continue;
        } else if (rc != FV_OK) {
            log_err("rc != FV_OK");
            goto err;
        }
        log_info("## parse request body suc");

        fv_http_out_t *out = (fv_http_out_t*)malloc(sizeof(fv_http_out_t));
        rc = fv_init_out_t(out, fd);
        check(rc == FV_OK, "zv_init_out_t");

        parse_uri(r->uri_start, r->uri_end - r->uri_start, filename, NULL);
        if (stat(filename, &sbuf) < 0) {
            do_error(fd, filename, "404", "Not Found", "ferver can't find the file");
            return;
        }

        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            do_error(fd, filename, "403", "Forbidden", "ferver can't read the file");
            return;
        }

        out->mtime = sbuf.st_mtime;
        fv_http_handle_header(r, out);
        check(list_empty(&(r->list)) == 1, "header list should be empty");
        if (out->status == 0)
        {
            out->status = FV_HTTP_OK;
        }

        serve_static(fd, filename, sbuf.st_size, out);
        log_info("# serve client(fd %d) suc", fd);

        fflush(stdout);
        if (!out->keep_alive) {
            log_info("when serve fd %d, request header has no keep_alive, ready to close", fd);
            close(fd);
        }
        free(out);
    }
    return;  // reuse tcp connections
err:
    log_info("err when serve fd %d, ready to close", fd);
    close(fd);
}
