#include "util.h"
#include "http.h"
#include "epoll.h"

extern struct epoll_event *events;

int main()
{
    int listenfd;
    int rc;
    struct sockaddr_in clientaddr;
    socklen_t inlen;

    listenfd = open_listenfd(SERVER_PORT);
    rc = make_socket_non_blocking(listenfd);
    check(rc == 0, "make_socket_non_blocking");

    int epfd = fv_epoll_create(0);
    struct epoll_event event;
    event.data.fd = listenfd;
    event.events = EPOLLIN | EPOLLET;
    fv_epoll_add(epfd, listenfd, &event);

    while (1)
    {
        log_info("ferver ready to wait");
        int n;
        n = fv_epoll_wait(epfd, events, MAXEVENTS, -1);

        int i;
        for (i = 0; i < n; ++i)
        {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLERR) ||
                    (!(events[i].events & EPOLLIN))) {
                log_err("epoll error");
                close(events[i].data.fd);
            } else if (listenfd == events[i].data.fd) {
                while (1) {
                    log_info("ferver ready to accept");
                    int infd = accept(listenfd, (struct sockaddr *)&clientaddr, &inlen);
                    if (infd == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            break;
                        } else {
                            log_err("accept");
                            break;
                        }
                    }
                    do_request(infd);
                    close(infd);
                }
            }
        }
    }

    return 0;
}
