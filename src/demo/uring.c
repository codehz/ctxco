#include "uring_impl.h"

#include <netinet/ip6.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define fatal(...)                                                                                                     \
    do {                                                                                                               \
        printf("Fatal: ");                                                                                             \
        printf(__VA_ARGS__);                                                                                           \
        abort();                                                                                                       \
    } while (0)

void client_co(void *priv);

void server_co(void *priv) {
    int server = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (server < 0) fatal("Failed to create socket: %s\n", strerror(errno));
    int option = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &option, sizeof option);

    struct sockaddr_in6 addr = {};
    addr.sin6_family         = AF_INET6;
    addr.sin6_port           = htons(8818);
    addr.sin6_addr           = in6addr_any;

    if (bind(server, (const struct sockaddr *) &addr, sizeof addr) < 0)
        fatal("Failed to bind port 8818: %s\n", strerror(errno));

    if (listen(server, 8) < 0) fatal("Failed to listen: %s\n", strerror(errno));

    printf("start accept ([::]:8818)\n");

    while (1) {
        struct sockaddr_in6 client;
        socklen_t len;
        ssize_t clifd = (ssize_t) ctxco_invoke(IORING_OP_ACCEPT, server, &client, &len, SOCK_CLOEXEC);
        if (clifd < 0) fatal("Failed to accept connection: %s\n", strerror(-clifd));
        ctxco_start(client_co, (void *) (ssize_t) clifd, 128 * 1024);
    }

    close(server);
    printf("closed\n");
}

void client_co(void *priv) {
    int client = (int) (int64_t) priv;
    while (1) {
        char buffer[32 * 1024];
        struct msghdr msg;
        struct iovec iov;
        iov.iov_base   = buffer;
        iov.iov_len    = sizeof buffer;
        msg.msg_iov    = &iov;
        msg.msg_iovlen = 1;
        ssize_t ret    = (ssize_t) ctxco_invoke(IORING_OP_RECVMSG, client, &msg, 0);
        if (ret <= 0) break;
        iov.iov_len = ret;
        ret         = (ssize_t) ctxco_invoke(IORING_OP_SENDMSG, client, &msg, 0);
        if (ret <= 0) break;
    }
    close(client);
}

int main() {
    poller_data_t poller_data;
    int ret = io_uring_queue_init(32, &poller_data.ring, 0);
    if (ret < 0) fatal("Failed to init io_uring: %s\n", strerror(errno));
    ctxco_init(io_uring_poller, &poller_data);

    ctxco_start(server_co, NULL, 128 * 1024);
    ctxco_loop();

    ctxco_deinit();
    io_uring_queue_exit(&poller_data.ring);
}