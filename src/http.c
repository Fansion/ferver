#include "http.h"
#include "dbg.h"
#include "rio.h"
#include <sys/stat.h>

#define MAXLINE 8192
#define SHORTLINE 512
#define ROOT "/home/frank/ustc_home"
#define CR '\r'
#define LF '\n'
#define CRLFCRLF "\r\n\r\n"

// Fast byte comparison. Inspired by Igor Sysoev's Nginx.
// big endian
#define fv_str3_cmp(m,c0,c1,c2,c3)     *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define fv_str30cmp(m, c0, c1, c2, c3) *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define fv_str4cmp(m, c0, c1, c2, c3)  *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

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

/*读取并忽略报头*/
void read_requesthdrs(rio_t *rio)
{
    check(rio != NULL, "rio==NULL");

    int rc;
    char buf[MAXLINE];
    rc = rio_readlineb(rio, buf, MAXLINE);
    if (rc == 0)
    {
        log_info("ready to close fd %d", rio->rio_fd);
        close(rio->rio_fd);
        return;
    }
    // 这儿为什么要检查rc == -EAGAIN???
    check((rc > 0 || rc == -EAGAIN), "read request line, rc > 0 || rc == -EAGAIN");
    while (strcmp(buf, "\r\n")) {
        // log_info("%s", buf);
        rc = rio_readlineb(rio, buf, MAXLINE);
        if (rc == 0)
        {
            log_info("ready to close fd %d", rio->rio_fd);
            close(rio->rio_fd);
            break;
        }
        check((rc > 0 || rc == -EAGAIN), "read request line, rc > 0 || rc == -EAGAIN");
    }
}

void parse_uri(char *uri, int uri_length,  char *filename, char *querystring)
{
    querystring = querystring;
    strcpy(filename, ROOT);
    strncat(filename, uri, uri_length);

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

void serve_static(int fd, char *filename, int filesize)
{
    log_info("filename = %s", filename);
    char header[MAXLINE];
    int n;

    const char *file_type;
    const char *dot_pos = rindex(filename, '.');
    file_type = get_file_type(dot_pos);

    sprintf(header, "HTTP/1.1 200 OK\r\n");
    sprintf(header, "%sServer: ferver\r\n", header);
    sprintf(header, "%sContent-type: %s\r\n", header, file_type);
    sprintf(header, "%sConnection: keep-alive\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, filesize);

    n = rio_writen(fd, header, strlen(header));
    check(n == (ssize_t)strlen(header), "rio_writen error");
    // use sendfile
    int srcfd = open(filename, O_RDONLY, 0);
    check(srcfd > 2, "open error");
    char *srcaddr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr != 0, "mmap error");
    close(srcfd);

    n = rio_writen(fd, srcaddr, filesize);
    check(n == filesize, "rio_writen error");

    munmap(srcaddr, filesize);
}

int fv_http_parse_request_line(fv_http_request_t *r)
{
    u_char ch, *p, *m;
    enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    } state;
    state = r->state;
    check(state == 0, "state should be 0");
    log_info("enter fv_http_parse_request_line, start addr=%d, last addr=%d, head length=%d", (int)r->pos, (int)r->last, (int)(r->last-r->pos));

    for (p = r->pos; p < (u_char *)r->last; ++p)
    {
        ch = *p;
        switch (state)
        {
        case sw_start:  /* HTTP methods: GET, HEAD, POST */
            log_info("in state sw_start");
            r->request_start = p;
            if (ch == CR || ch == LF)
                break;
            if ((ch < 'A' || ch > 'Z') && ch != '_')
            {
                return FV_HTTP_PARSE_INVALID_METHOD;
            }
            state = sw_method;
            break;
        case sw_method:
            if (ch == ' ') {
                r->method_end = p;
                m = r->request_start;
                switch (p - m)
                {
                case 3:
                    if (fv_str3_cmp(m, 'G', 'E', 'T', ' '))
                    {
                        r->method = FV_HTTP_GET;
                        break;
                    }
                    break;
                case 4:
                    if (fv_str30cmp(m, 'P', 'O', 'S', 'T'))
                    {
                        r->method = FV_HTTP_POST;
                        break;
                    }
                    if (fv_str4cmp(m, 'H', 'E', 'A', 'D'))
                    {
                        r->method = FV_HTTP_HEAD;
                        break;
                    }
                    break;
                default:
                    r->method = FV_HTTP_UNKNOWN;
                    break;
                }
                state = sw_spaces_before_uri;
                break;
            }
            if ((ch < 'A' || ch > 'Z') && ch != '_')
            {
                return FV_HTTP_PARSE_INVALID_METHOD;
            }
            break;
        case sw_spaces_before_uri:  /* space* before URI */
            if (ch == '/')
            {
                r->uri_start = p;
                state = sw_after_slash_in_uri;
                break;
            }
            switch (ch) {
            case ' ':
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_after_slash_in_uri:
            switch (ch) {
            case ' ':
                r->uri_end = p;
                state = sw_http;
                break;
            default:
                break;
            }
            break;
        case sw_http:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                state = sw_http_H;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_H:
            switch (ch) {
            case 'T':
                state = sw_http_HT;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_HT:
            switch (ch) {
            case 'T':
                state = sw_http_HTT;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_HTT:
            switch (ch) {
            case 'P':
                state = sw_http_HTTP;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_HTTP:
            switch (ch) {
            case '/':
                state = sw_first_major_digit;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_first_major_digit: /* first digit of major HTTP version */
            if (ch < '1' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_major = ch - '0';
            state = sw_major_digit;
            break;
        case sw_major_digit: /* major HTTP version or not */
            if (ch == '.')
            {
                state = sw_first_minor_digit;
                break;
            }
            if (ch < '0' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_major = r->http_major * 10 + ch - '0';
            break;
        case sw_first_minor_digit:  /* first digit of minor HTTP version */
            if (ch < '0' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_minor = ch - '0';
            state = sw_minor_digit;
            break;
        case sw_minor_digit:
            if (ch == CR) {
                state = sw_almost_done;
                break;
            }
            if (ch == LF)
            {
                goto done;
            }
            if (ch == ' ')
            {
                state = sw_spaces_after_digit;
                break;
            }
            if (ch < '0' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_minor = r->http_minor * 10 + ch - '0';
            break;
        case sw_spaces_after_digit:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_almost_done:
            r->request_end = p - 1;
            switch (ch) {
            case LF:
                goto done;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
        }
    }
    r->pos = p;
    r->state = state;
    return FV_AGAIN;
done:
    r->pos = p + 1;
    if (r->request_end == NULL)
    {
        r->request_end = p;
    }
    r->state = sw_start;
    return FV_OK;
}

int fv_http_parse_request_body(fv_http_request_t *r)
{
    u_char ch, *p;
    enum {
        sw_start = 0,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    } state;
    state = r->state;
    check(state == 0, "state should be 0");
    log_info("enter fv_http_parse_request_body,  start addr=%d, last addr=%d, body length=%d", (int)r->pos, (int)r->last, (int)(r->last-r->pos));
    if (r->pos == r->last)       // empty body return directly
        return FV_OK;
    for (p = r->pos; p < (u_char *)r->last; ++p)
    {
        ch = *p;
        switch (state)
        {
        case sw_start:
            switch (ch)
            {
                case CR:
                    state = sw_cr;
                    break;
                default:
                    break;
            }
            break;
        case sw_cr:
            switch (ch)
            {
                case LF:
                    state = sw_crlf;
                    break;
                default:
                    return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_crlf:
            switch (ch)
            {
                case CR:
                    state = sw_crlfcr;
                    break;
                default:
                    state = sw_start;
                    break;
            }
            break;
        case sw_crlfcr:
            switch (ch)
            {
                case LF:
                    goto done;
                default:
                    return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        }
    }
    r->pos = p;
    r->state = state;

    return FV_AGAIN;
done:
    r->pos = p+1;
    r->state = sw_start;
    return FV_OK;
}

int fv_init_request_t(fv_http_request_t *r, int fd)
{
    r->fd = fd;
    r->pos = r->last = r->buf;
    r->state = 0;

    return FV_OK;
}

void do_request(void *ptr)
{
    fv_http_request_t *r = (fv_http_request_t *)ptr;
    int fd = r->fd;
    int rc;
    char filename[SHORTLINE];
    struct stat sbuf;
    int n;

    log_info("ready to serve client(fd %d)", fd);
    for (;;)
    {
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

        log_info("ready to parse request line");
        rc = fv_http_parse_request_line(r);
        if (rc == FV_AGAIN) {
            continue;
        } else if(rc != FV_OK) {
            log_err("rc != FV_OK, ready to close fd %d", fd);
            goto err;
        }

        log_info("request method == %.*s",(int)(r->method_end - r->request_start), (char *)r->request_start);
        log_info("request uri == %.*s", (int)(r->uri_end - r->uri_start), (char *)r->uri_start);

        log_info("ready to parse request body");
        rc  = fv_http_parse_request_body(r);
        if (rc == FV_AGAIN) {
            continue;
        } else if(rc != FV_OK) {
            log_err("rc != FV_OK, ready to close fd %d", fd);
            goto err;
        }

        parse_uri(r->uri_start, r->uri_end - r->uri_start, filename, NULL);
        if (stat(filename, &sbuf) < 0) {
            do_error(fd, filename, "404", "Not Found", "ferver can't find the file");
            return;
        }

        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            do_error(fd, filename, "403", "Forbidden", "ferver can't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
        log_info("serve_static suc");
    }
    return;  // reuse tcp connections
err:
    close(fd);
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
    sprintf(header, "%sConnection: keep-alive\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));
    log_info("header  = \n %s\n", header);
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    log_info("leave do_error");
}
