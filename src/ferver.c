#include "util.h"
#include "http.h"
#include "epoll.h"
#include "threadpool.h"

extern struct epoll_event *events;

int main()
{
    /*
     *initialize listening socket
     */
    int listenfd;
    int rc;
    struct sockaddr_in clientaddr;
    socklen_t inlen;

    listenfd = open_listenfd(SERVER_PORT);
    rc = make_socket_non_blocking(listenfd);
    check(rc == 0, "make_socket_non_blocking");

    /*
     *create epoll and add listenfd to epfd
     */
    int epfd = fv_epoll_create(0);
    struct epoll_event event;
    event.data.fd = listenfd;
    event.events = EPOLLIN | EPOLLET;
    fv_epoll_add(epfd, listenfd, &event);

    /*
     *create thread pool
     */
    fv_threadpool_t *tp = threadpool_init(THREAD_NUM);

    while (1)
    {
        log_info("ferver(fd %d) ready to wait", listenfd);
        /* epoll_wait loop */
        int n;
        n = fv_epoll_wait(epfd, events, MAXEVENTS, -1);

        int i;
        for (i = 0; i < n; ++i)
        {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLERR) ||
                    (!(events[i].events & EPOLLIN))) {
                log_err("epoll error(fd %d)", events[i].data.fd);
                close(events[i].data.fd);
            }
            if (listenfd == events[i].data.fd) {
                while (1) {
                    log_info("ferver(fd %d) ready to accept", listenfd);
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
                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    fv_epoll_add(epfd, infd, &event);
                }
            } else {
                /*
                 do_request(infd);
                 close(infd);
                 */
                rc = threadpool_add(tp, do_request, (void *)events[i].data.fd);
            }
        }
    }

    return 0;
}
