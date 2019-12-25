#include "../ctxco/ctxco.h"

#include "epoll_impl.h"
#include <errno.h>
#include <netinet/ip6.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int server = 0;

void signal_catcher() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTSTP);
    sigaddset(&sigset, SIGCONT);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
    int sfd = signalfd(-1, &sigset, SFD_NONBLOCK | SFD_CLOEXEC);
    while (1) {
        uint64_t ret = (uint64_t) ctxco_invoke(EPOLLIN | EPOLLERR, &(epoll_req_t){sfd});
        if (ret != EPOLLIN) {
            printf("Failed to invoke epoll from signalfd: %s\n", strerror(errno));
            goto end;
        }
        union {
            struct signalfd_siginfo info;
            char buf[sizeof(struct signalfd_siginfo)];
        } u;
        if (read(sfd, u.buf, sizeof u.buf) == -1) {
            printf("Failed to read from signalfd: %s\n", strerror(errno));
            goto end;
        }
        switch (u.info.ssi_signo) {
        case SIGINT: printf("SIGINT\n"); goto end;
        case SIGTSTP: printf("Go background\n"); break;
        case SIGCONT: printf("Go foreground\n"); break;
        default: printf("UNKNOWN\n"); goto end;
        }
    }
end:
    sigemptyset(&sigset);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
    close(sfd);
    shutdown(server, SHUT_RDWR);
}

#define fatal(...)                                                                                                     \
    do {                                                                                                               \
        printf("Fatal: ");                                                                                             \
        printf(__VA_ARGS__);                                                                                           \
        abort();                                                                                                       \
    } while (0)

void client_co(void *priv);

void server_co(void *priv) {
    server = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
    if (server < 0) fatal("Failed to create socket: %s\n", strerror(errno));

    struct sockaddr_in6 addr = {};
    addr.sin6_family         = AF_INET6;
    addr.sin6_port           = htons(8818);
    addr.sin6_addr           = in6addr_any;

    if (bind(server, (const struct sockaddr *) &addr, sizeof addr) < 0)
        fatal("Failed to bind port 8818: %s\n", strerror(errno));

    if (listen(server, 8) < 0) fatal("Failed to listen: %s\n", strerror(errno));

    printf("start accept ([::1]:8818)\n");

    while (1) {
        uint64_t ret = (uint64_t) ctxco_invoke(EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR, &(epoll_req_t){server});
        if (ret != EPOLLIN) break;
        struct sockaddr_in6 client;
        socklen_t len;
        int clifd = accept4(server, (struct sockaddr *) &client, &len, SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (clifd < 0) fatal("Failed to accept connection: %d %s\n", errno, strerror(errno));
        ctxco_start(client_co, (void *) (int64_t) clifd, 128 * 1024);
        ctxco_yield();
    }

    close(server);
    printf("closed\n");
}

void client_co(void *priv) {
    int client = (int) (int64_t) priv;
    while (1) {
        uint64_t ret = (uint64_t) ctxco_invoke(EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR, &(epoll_req_t){client});
        if (ret != EPOLLIN) break;
        char buffer[32 * 1024];
        size_t offset = 0;
        while (1) {
            int len = recv(client, buffer, sizeof buffer, 0);
            if (len <= 0) {
                if (errno == EWOULDBLOCK) break;
                printf("recv err: %s\n", strerror(errno));
                goto end;
            }
            while (1) {
                int written = send(client, buffer, len - offset, 0);
                if (written <= 0 && errno != EWOULDBLOCK) {
                    printf("send err: %s\n", strerror(errno));
                    goto end;
                }
                offset += written;
                if (offset == len) break;
                ret = (uint64_t) ctxco_invoke(EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR, &(epoll_req_t){client});
                if (ret != EPOLLOUT) break;
            };
        }
    }
end:
    close(client);
}

int main() {
    poller_data_t epoll_data = {};
    epoll_data.epfd          = epoll_create1(EPOLL_CLOEXEC);
    ctxco_init(epoll_poller, &epoll_data);

    ctxco_start(server_co, NULL, 128 * 1024);
    ctxco_start(signal_catcher, NULL, 128 * 1024);

    ctxco_loop();

    ctxco_deinit();
    close(epoll_data.epfd);
}