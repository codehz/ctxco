#include "../ctxco/ctxco.h"

#include "epoll_impl.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

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
    if (ret == -1) { printf("Failed to read from timerfd: %s\n", strerror(errno)); }

    close(tfd);
}

int main() {
    poller_data_t epoll_data = {};
    epoll_data.epfd          = epoll_create1(EPOLL_CLOEXEC);
    ctxco_init(epoll_poller, &epoll_data);

    srand(time(NULL));

    for (size_t i = 0; i < 20; i++) ctxco_start(timer_co, (void *) i, 128 * 1024);

    ctxco_loop();

    ctxco_deinit();
    close(epoll_data.epfd);
}