#include "util.h"

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
        return -1;
    /* Listenfd will be an endpoint for all requests to port on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;

    return listenfd;
}

/*
    make a socket non blocking. If a listen socket is a blocking socket, after it comes out from epoll and accepts the last connection, the next accpet will block, which is not what we want
*/
int make_socket_non_blocking(int fd)
{
    int flags, s;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        log_err("fcntl get");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1)
    {
        log_err("fcntl set");
        return -1;
    }
    return 0;
}

/*
 *  从配置文件中读取相应的参数
 */
int read_conf(char *filename, fv_conf_t *cf, char *buf,  int len)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        log_err("cannot open conf file: %s", filename);
        return FV_CONF_ERROR;
    }
    char *delim_pos;
    int line_len = 0, max_len = len;
    char *cur_pos = buf;

    while (fgets(cur_pos, max_len, fp))             // read a line
    {
        delim_pos = strstr(cur_pos, DELIM);
        line_len = strlen(cur_pos);
        // debug("read one line from conf: %s len=%d", cur_pos, line_len);
        if (!delim_pos)
        {
            return FV_CONF_ERROR;
        }
        if (cur_pos[line_len - 1] == '\n')          // to get root without '\n'
        {
            cur_pos[line_len - 1] = '\0';
        }
        if (strncmp("root", cur_pos, 4) == 0)
        {
            cf->root = delim_pos + 1;
        }
        if (strncmp("port", cur_pos, 4) == 0)
        {
            cf->port = atoi(delim_pos + 1);
        }
        if (strncmp("thread_num", cur_pos, 10) == 0)
        {
            cf->thread_num = atoi(delim_pos + 1);
        }
        cur_pos += line_len;
        max_len -= line_len;
    }
    fclose(fp);
    return FV_CONF_OK;
}
