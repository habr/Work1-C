#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ev.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

struct echo_watcher {
    struct ev_io io;
    int fd;
};

static void echo_cb(EV_P_ struct ev_io *watcher, int revents);
static void accept_cb(EV_P_ struct ev_io *watcher, int revents);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    struct ev_loop *loop = EV_DEFAULT;
    int sd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 1;
    }

    if (listen(sd, SOMAXCONN) != 0) {
        perror("listen");
        return 1;
    }

    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, sd, EV_READ);
    ev_io_start(loop, &w_accept);

    ev_run(loop, 0);

    return 0;
}

static void accept_cb(EV_P_ struct ev_io *watcher, int revents) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sd = accept(watcher->fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sd < 0) {
        perror("accept error");
        return;
    }

    struct echo_watcher *client_watcher = (struct echo_watcher*) malloc(sizeof(struct echo_watcher));
    ev_io_init(&client_watcher->io, echo_cb, client_sd, EV_READ);
    ev_io_start(loop, &client_watcher->io);
}

static void echo_cb(EV_P_ struct ev_io *watcher, int revents) {
    char buffer[BUFFER_SIZE];
    ssize_t read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);

    if (read < 0) {
        perror("read error");
        return;
    } else if (read == 0) {
        ev_io_stop(loop, watcher);
        close(watcher->fd);
        free(watcher);
        return;
    } else {
        send(watcher->fd, buffer, read, 0);
    }
}
