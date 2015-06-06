#include "util.h"
#include "http.h"

int main()
{
    int listenfd;
    int rc;
    struct sockaddr_in clientaddr;
    socklen_t inlen;

    listenfd = open_listenfd(SERVER_PORT);

    while(1)
    {
        log_info("ferver ready to accept");
        int infd = accept(listenfd, (struct sockaddr*)&clientaddr, &inlen);
        if(infd == -1) {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                continue;
            } else {
                log_err("accept");
                break;
            }
        }
        do_request(infd);
        close(infd);
    }


    return 0;
}
