#include "../ctxco/ctxco.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

typedef struct poller_data_t {
    int epfd;
    int count;
} poller_data_t, *poller_data_ref_t;

typedef struct epoll_req_t {
    int fd;
    ctxco_impl_t rco;
} epoll_req_t, *epoll_req_ref_t;

static void epoll_poller(void *priv, ctxco_request_ref_t co) {
    poller_data_ref_t ref = (poller_data_ref_t) priv;

    if (co == NULL || co == CTXCO_BLOCK) {
        if (ref->count == 0) return;
        struct epoll_event evts[8];
        int nfds = epoll_wait(ref->epfd, evts, 8, co == CTXCO_BLOCK ? -1 : 1);
        if (nfds == -1) {
            printf("Failed to epoll: %s\n", strerror(errno));
            abort();
        }
        for (int i = 0; i < nfds; i++) {
            ref->count--;
            epoll_req_ref_t req = (epoll_req_ref_t) evts[i].data.ptr;
            ctxco_resume(req->rco, (void *) (uint64_t) evts[i].events);
            epoll_ctl(ref->epfd, EPOLL_CTL_DEL, req->fd, NULL);
        }
    } else {
        epoll_req_ref_t req = va_arg(co->arg, epoll_req_ref_t);
        req->rco            = co->ctx;
        struct epoll_event event;
        event.events   = co->op;
        event.data.ptr = req;
        int ret        = epoll_ctl(ref->epfd, EPOLL_CTL_ADD, req->fd, &event);
        if (ret == -1) {
            ctxco_resume(co->ctx, NULL);
        } else
            ref->count++;
    }
}