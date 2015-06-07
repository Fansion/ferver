#include "http.h"
#include "dbg.h"
#include "rio.h"
#include <sys/stat.h>

#define MAXLINE 8192
#define SHORTLINE 512
#define ROOT "/home/frank/ustc_home"

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

void parse_uri(char *uri, char *filename, char *querystring)
{
    querystring = querystring;
    strcpy(filename, ROOT);
    strcat(filename, uri);

    char *last_dot = rindex(filename, '.');

    if (last_dot == NULL && filename[strlen(filename) - 1] != '/') {
        strcat(filename, "/");
    }
    if (uri[strlen(uri) - 1] == '/') {
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

void do_request(void *infd)
{
    int fd = (int)infd;
    int rc;
    rio_t rio;
    char method[SHORTLINE], uri[SHORTLINE], version[SHORTLINE];
    char buf[MAXLINE];
    char filename[SHORTLINE];
    struct stat sbuf;

    rio_readinitb(&rio, fd);
    for (;;) // reuse tcp connections
    {
        rc = rio_readlineb(&rio, buf, MAXLINE);
        if (rc == 0)
        {
            log_info("ready to close fd %d", fd);
            close(fd);
            break;
        }
        if (rc == -EAGAIN)  // 当数据读完之后需要break跳出循环
            break;
        check((rc > 0), "read request line, rc should > 0");
        sscanf(buf, "%s %s %s", method, uri, version);  // if buf is empty, method donot change
        log_info("request %s from fd %d", uri, fd);
        if (strcasecmp(method, "GET")) {
            log_err("req line = %s", buf);
            log_err("method = %s", method);
            log_err("buf = %s", buf);
            log_err("rc = %d", rc);
            do_error(fd, method, "501", "Not Implemented", "ferver doesn't support");
            return;
        }
        read_requesthdrs(&rio);
        parse_uri(uri, filename, NULL);

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