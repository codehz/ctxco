#include "../ctxco/ctxco.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

typedef struct poller_data_t {
    int epfd;
    int count;
} poller_data_t, *poller_data_ref_t;

typedef struct epoll_req_t {
    int fd;
    int events;
} epoll_req_t, *epoll_req_ref_t;

void epoll_poller(void *priv, ctxco_request_ref_t co) {
    poller_data_ref_t ref = (poller_data_ref_t) priv;

    if (co == NULL || co == CTXCO_BLOCK) {
        struct epoll_event evts[8];
        int nfds = epoll_wait(ref->epfd, evts, 8, co == CTXCO_BLOCK ? -1 : 1);
        if (nfds == -1) {
            printf("Failed to epoll: %s", strerror(errno));
            abort();
        }
        for (int i = 0; i < nfds; i++) {
            ctxco_resume((ctxco_impl_t) evts[i].data.ptr, (void *) (uint64_t) evts[i].events);
        }
    } else {
        epoll_req_ref_t req = (epoll_req_ref_t) co->priv;
        struct epoll_event event;
        event.events   = req->events | EPOLLONESHOT;
        event.data.ptr = co->ctx;
        int ret        = epoll_ctl(ref->epfd, EPOLL_CTL_ADD, req->fd, &event);
        if (ret == -1) {
            printf("Failed to epoll add: %s", strerror(errno));
            ctxco_resume(co->ctx, NULL);
        }
    }
}

void timer_co(void *priv) {
    int tfd                 = timerfd_create(1, TFD_NONBLOCK | TFD_CLOEXEC);
    struct itimerspec timer = {};
    timer.it_value.tv_nsec  = 1000000 * (rand() % 800 + 200);
    timerfd_settime(tfd, 0, &timer, NULL);

    epoll_req_t req = {tfd, EPOLLIN};

    printf("start invoke [%ld]\n", (size_t) priv);

    ctxco_invoke(&req);

    printf("end invoke [%ld]\n", (size_t) priv);

    union {
        uint64_t real;
        char buf[sizeof(uint64_t)];
    } u;
    int ret = read(tfd, u.buf, sizeof u.buf);
    if (ret == -1) { printf("Failed to read from timerfd: %s", strerror(errno)); }

    close(tfd);
}

int main() {
    poller_data_t epoll_data = {};
    epoll_data.epfd          = epoll_create1(EPOLL_CLOEXEC);
    ctxco_init(epoll_poller, &epoll_data);

    srand(time(NULL));

    for (size_t i = 0; i < 100; i++) ctxco_start(timer_co, (void *) i, 128 * 1024);

    ctxco_loop();

    ctxco_deinit();
    close(epoll_data.epfd);
}