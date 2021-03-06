#include "../ctxco/ctxco.h"

#include <errno.h>
#include <liburing.h>

typedef struct poller_data_t {
    struct io_uring ring;
    int count;
} poller_data_t, *poller_data_ref_t;

#define CQE_BATCH 8

void io_uring_poller(void *priv, ctxco_request_ref_t co) {
    poller_data_ref_t ref = (poller_data_ref_t) priv;

    if (co == NULL) {
        if (ref->count == 0) return;
        unsigned i;
        struct io_uring_cqe *cqe;
        io_uring_for_each_cqe(&ref->ring, i, cqe) {
            io_uring_cqe_seen(&ref->ring, cqe);
            ctxco_impl_t ctx = (ctxco_impl_t) io_uring_cqe_get_data(cqe);
            ctxco_resume(ctx, (void *) (ssize_t) cqe->res);
            ref->count--;
        }
    } else if (co == CTXCO_BLOCK) {
        if (ref->count == 0) return;
        io_uring_submit(&ref->ring);
        struct io_uring_cqe *cqe = NULL;
        int ret                  = io_uring_wait_cqe(&ref->ring, &cqe);
        if (ret < 0) { return; }
        io_uring_cqe_seen(&ref->ring, cqe);
        ctxco_impl_t ctx = (ctxco_impl_t) io_uring_cqe_get_data(cqe);
        ctxco_resume(ctx, (void *) (ssize_t) cqe->res);
        ref->count--;
        io_uring_poller(priv, NULL);
    } else {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ref->ring);
        if (!sqe) {
            io_uring_submit(&ref->ring);
            sqe = io_uring_get_sqe(&ref->ring);
            if (!sqe) {
                ctxco_resume(co->ctx, (void *) (ssize_t) -EAGAIN);
                return;
            }
        }
        switch (co->op) {
        case IORING_OP_READV: {
            int fd                     = va_arg(co->arg, int);
            const struct iovec *iovecs = va_arg(co->arg, const struct iovec *);
            unsigned nr_vecs           = va_arg(co->arg, unsigned);
            off_t offset               = va_arg(co->arg, off_t);
            io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
        } break;
        case IORING_OP_READ_FIXED: {
            int fd              = va_arg(co->arg, int);
            void *buf           = va_arg(co->arg, void *);
            unsigned int nbytes = va_arg(co->arg, unsigned);
            off_t offset        = va_arg(co->arg, off_t);
            int buf_index       = va_arg(co->arg, int);
            io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
        } break;
        case IORING_OP_WRITEV: {
            int fd                     = va_arg(co->arg, int);
            const struct iovec *iovecs = va_arg(co->arg, const struct iovec *);
            unsigned nr_vecs           = va_arg(co->arg, unsigned);
            off_t offset               = va_arg(co->arg, off_t);
            io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
        } break;
        case IORING_OP_WRITE_FIXED: {
            int fd              = va_arg(co->arg, int);
            const void *buf     = va_arg(co->arg, void const *);
            unsigned int nbytes = va_arg(co->arg, unsigned);
            off_t offset        = va_arg(co->arg, off_t);
            int buf_index       = va_arg(co->arg, int);
            io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
        } break;
        case IORING_OP_RECVMSG: {
            int fd             = va_arg(co->arg, int);
            struct msghdr *msg = va_arg(co->arg, struct msghdr *);
            unsigned int flags = va_arg(co->arg, unsigned);
            io_uring_prep_recvmsg(sqe, fd, msg, flags);
        } break;
        case IORING_OP_SENDMSG: {
            int fd                   = va_arg(co->arg, int);
            const struct msghdr *msg = va_arg(co->arg, struct msghdr const *);
            unsigned int flags       = va_arg(co->arg, unsigned);
            io_uring_prep_sendmsg(sqe, fd, msg, flags);
        } break;
        case IORING_OP_POLL_ADD: {
            int fd          = va_arg(co->arg, int);
            short poll_mask = va_arg(co->arg, int);
            io_uring_prep_poll_add(sqe, fd, poll_mask);
        } break;
        case IORING_OP_POLL_REMOVE: {
            void *user_data = va_arg(co->arg, void *);
            io_uring_prep_poll_remove(sqe, user_data);
        } break;
        case IORING_OP_FSYNC: {
            int fd                   = va_arg(co->arg, int);
            unsigned int fsync_flags = va_arg(co->arg, unsigned);
            io_uring_prep_fsync(sqe, fd, fsync_flags);
        } break;
        case IORING_OP_NOP: {
            io_uring_prep_nop(sqe);
        } break;
        case IORING_OP_TIMEOUT: {
            struct __kernel_timespec *ts = va_arg(co->arg, struct __kernel_timespec *);
            unsigned count               = va_arg(co->arg, unsigned);
            unsigned flags               = va_arg(co->arg, unsigned);
            io_uring_prep_timeout(sqe, ts, count, flags);
        } break;
        case IORING_OP_TIMEOUT_REMOVE: {
            __u64 user_data    = va_arg(co->arg, uint64_t);
            unsigned int flags = va_arg(co->arg, unsigned);
            io_uring_prep_timeout_remove(sqe, user_data, flags);
        } break;
        case IORING_OP_ACCEPT: {
            int fd                = va_arg(co->arg, int);
            struct sockaddr *addr = va_arg(co->arg, struct sockaddr *);
            socklen_t *addrlen    = va_arg(co->arg, socklen_t *);
            int flags             = va_arg(co->arg, int);
            io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        } break;
        case IORING_OP_ASYNC_CANCEL: {
            void *user_data = va_arg(co->arg, void *);
            int flags       = va_arg(co->arg, unsigned);
            io_uring_prep_cancel(sqe, user_data, flags);
        } break;
        case IORING_OP_LINK_TIMEOUT: {
            struct __kernel_timespec *ts = va_arg(co->arg, struct __kernel_timespec *);
            unsigned flags               = va_arg(co->arg, unsigned);
            io_uring_prep_link_timeout(sqe, ts, flags);
        } break;
        case IORING_OP_CONNECT: {
            int fd                = va_arg(co->arg, int);
            struct sockaddr *addr = va_arg(co->arg, struct sockaddr *);
            socklen_t addrlen     = va_arg(co->arg, socklen_t);
            io_uring_prep_connect(sqe, fd, addr, addrlen);
        } break;
        case IORING_OP_FILES_UPDATE: {
            int *fds            = va_arg(co->arg, int *);
            unsigned int nr_fds = va_arg(co->arg, unsigned);
            int offset          = va_arg(co->arg, int);
            io_uring_prep_files_update(sqe, fds, nr_fds, offset);
        } break;
        case IORING_OP_FALLOCATE: {
            int fd       = va_arg(co->arg, int);
            int mode     = va_arg(co->arg, int);
            off_t offset = va_arg(co->arg, off_t);
            off_t len    = va_arg(co->arg, off_t);
            io_uring_prep_fallocate(sqe, fd, mode, offset, len);
        } break;
        case IORING_OP_OPENAT: {
            int dfd          = va_arg(co->arg, int);
            const char *path = va_arg(co->arg, char const *);
            int flags        = va_arg(co->arg, int);
            mode_t mode      = va_arg(co->arg, mode_t);
            io_uring_prep_openat(sqe, dfd, path, flags, mode);
        } break;
        case IORING_OP_CLOSE: {
            int fd = va_arg(co->arg, int);
            io_uring_prep_close(sqe, fd);
        } break;
        case IORING_OP_READ: {
            int fd              = va_arg(co->arg, int);
            void *buf           = va_arg(co->arg, void *);
            unsigned int nbytes = va_arg(co->arg, unsigned);
            off_t offset        = va_arg(co->arg, off_t);
            io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        } break;
        case IORING_OP_WRITE: {
            int fd              = va_arg(co->arg, int);
            void *buf           = va_arg(co->arg, void *);
            unsigned int nbytes = va_arg(co->arg, unsigned);
            off_t offset        = va_arg(co->arg, off_t);
            io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        } break;
        case IORING_OP_STATX: {
            int dfd                = va_arg(co->arg, int);
            const char *path       = va_arg(co->arg, char const *);
            int flags              = va_arg(co->arg, int);
            unsigned int mask      = va_arg(co->arg, unsigned);
            struct statx *statxbuf = va_arg(co->arg, struct statx *);
            io_uring_prep_statx(sqe, dfd, path, flags, mask, statxbuf);
        } break;
        default: io_uring_prep_nop(sqe);
        }
        io_uring_sqe_set_data(sqe, co->ctx);
        ref->count++;
    }
}