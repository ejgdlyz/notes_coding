#include <event2/event.h>
#include <event2/bufferevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

void read_callback(struct bufferevent *bev, void *ctx) {
    char buffer[256];
    int n;

    // 读取数据
    while ((n = bufferevent_read(bev, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, n, stdout);
    }
}

void write_callback(struct bufferevent *bev, void *ctx) {
    printf("Data successfully written to the socket!\n");
}

void event_callback(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR) {
        perror("Error from bufferevent");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

int main() {
    struct event_base *base = event_base_new();

    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345); // 服务器端口
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // 服务器 IP 地址

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(socket_fd);
        return 1;
    }

    struct bufferevent *bev = bufferevent_socket_new(base, socket_fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!\n");
        close(socket_fd);
        return 1;
    }

    bufferevent_setcb(bev, read_callback, write_callback, event_callback, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);  // 启用读写事件

    event_base_dispatch(base);

    event_base_free(base);
    return 0;
}