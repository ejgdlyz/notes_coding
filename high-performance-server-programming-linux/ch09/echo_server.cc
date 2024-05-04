#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <iostream>
#include <pthread.h>

/** 
 * 同时处理 TCP 和 UDP 服务
 * 从 bind 系统调用的参数可知，一个 socket 只能与一个 sock 地址绑定，即一个 socket 只能监听一个端口。
 * 服务器要想同时处理多个端口，就需要创建多个 socket, 并将它们分别绑定到各个端口。因此，服务器程序就需要管理多个监听 socket，即IO复用技术。 
 * 即使是同一个端口, TCP 和 UDP 请求也需要分别创建不同的 socket，即流式 socket 和 数据报 socket。
 * 
 * 同时处理一个端口上的 TCP 和 UDP 请求的 回射服务器
*/

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char const *argv[]) {
     if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    // 创建 TCP socket， 并将其绑定到端口 port 上
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert( ret != -1);
    
    // 创建 UDP socket，并将其绑定到端口 port 上
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);
    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(udpfd >= 0);

    ret = bind(udpfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    // 注册 TCP socket 和 UDP socket 上的可读事件
    addfd(epollfd, listenfd);
    addfd(epollfd, udpfd);

    while (1) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number <= 0) {
            std::cout << "epoll failure" << std::endl;
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
                addfd(epollfd, connfd);
            } else if (sockfd == udpfd) {
                char buf[UDP_BUFFER_SIZE];
                memset(buf, 0, sizeof(buf));
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                ret = recvfrom(udpfd, buf, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);

                if (ret > 0) {
                    sendto(udpfd, buf, UDP_BUFFER_SIZE - 1, 0, (struct  sockaddr*)&client_addr, client_len);
                }
            } else if (events[i].events & EPOLLIN) {
                char buf[UDP_BUFFER_SIZE];
                while (1) {
                    memset(buf, 0, sizeof(buf));
                    ret = recv(sockfd, buf, TCP_BUFFER_SIZE - 1, 0);
                    if (ret < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        close(sockfd);
                        break;
                    } else if (ret == 0) {
                        close(sockfd);
                    } else {
                        send(sockfd, buf, ret, 0);
                    }
                }
            } else {
                std::cout << "something else happened" << std::endl;
            }
        }
    }
    
    close(listenfd);
    return 0;
}

/* 
errno == EAGAIN || errno == EWOULDBLOCK
在非阻塞模式下，如果调用 recv 函数而没有数据可读（实际上该socket的接收缓冲区为空），
或者调用 send 函数而没有足够的缓冲区空间可写（实际上是发送缓冲区已满），
那么这两个函数调用会立即返回一个错误，并将错误码设置为 EAGAIN 或 EWOULDBLOCK。

这段代码中，服务器无论何时试图从空的接收缓存区读取数据或写入满的发送缓存区，
函数都将返回，然后通过设置的 EAGAIN 或 EWOULDBLOCK 错误值通知服务器发生了何种情况，以便服务器做出适当的调整。
*/                        