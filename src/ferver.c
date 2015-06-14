#include "http.h"
#include "epoll.h"
#include "threadpool.h"

#define CONF "ferver.conf"

extern struct epoll_event *events;

int main()
{
    int rc;
    /*
     *read configure file
     */
    char conf_buf[BUFFLEN];
    fv_conf_t cf;
    rc = read_conf(CONF, &cf, conf_buf, BUFFLEN);
    check(rc == FV_CONF_OK, "read conf err");
    log_info("root:%s", (char *)cf.root);
    log_info("port:%d", cf.port);
    log_info("thread_num:%d", cf.thread_num);
    /*
     *initialize listening socket
     */
    int listenfd;
    struct sockaddr_in clientaddr;
    // initialize clientaddr and inlen to solve "accept Invalid argument" bug
    socklen_t inlen = 1;
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));

    listenfd = open_listenfd(cf.port);
    rc = make_socket_non_blocking(listenfd);
    check(rc == 0, "make_socket_non_blocking");

    /*
     *create epoll and add listenfd to epfd
     */
    int epfd = fv_epoll_create(0);
    struct epoll_event event;

    fv_http_request_t *r = (fv_http_request_t *)malloc(sizeof(fv_http_request_t));
    fv_init_request_t(r, listenfd, &cf);
    event.data.ptr = (void *)r;
    event.events = EPOLLIN | EPOLLET;
    fv_epoll_add(epfd, listenfd, &event);

    /*
     *create thread pool
     */
    fv_threadpool_t *tp = threadpool_init(cf.thread_num);

    while (1)
    {
        log_info("ferver(fd %d) ready to wait", listenfd);
        fflush(stdout);
        /* epoll_wait loop */
        int n;
        n = fv_epoll_wait(epfd, events, MAXEVENTS, -1);

        int i, fd;
        for (i = 0; i < n; ++i)
        {
            fv_http_request_t *r = (fv_http_request_t *)events[i].data.ptr;
            fd = r->fd;
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLERR) ||
                    (!(events[i].events & EPOLLIN))) {
                log_err("epoll error(fd %d)", fd);
                close(fd);
                continue;
            }
            if (listenfd == fd) {
                log_info("# ferver(fd %d) ready to accept", listenfd);
                fflush(stdout);
                int infd = accept(listenfd, (struct sockaddr *)&clientaddr, &inlen);
                if (infd == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        log_info("the socket(listenfd %d) is marked nonblocking and no connections are present to be accepted", listenfd);
                        break;
                    } else {
                        log_err("accept");
                        break;
                    }
                }
                rc = make_socket_non_blocking(infd);
                check(rc == 0, "make_socket_non_blocking");
                log_info("new client connection(fd %d)", infd);

                fv_http_request_t *r = (fv_http_request_t *)malloc(sizeof(fv_http_request_t));
                fv_init_request_t(r, infd, &cf);
                event.data.ptr = (void *)r;
                event.events = EPOLLIN | EPOLLET;
                fv_epoll_add(epfd, infd, &event);
                log_info("# end accept");
                fflush(stdout);
            } else {
                /*
                 do_request(infd);
                 close(infd);
                 */
                log_info("new data from client(fd %d)", fd);
                rc = threadpool_add(tp, do_request, (void *)events[i].data.ptr);
            }
        }
    }

    return 0;
}
