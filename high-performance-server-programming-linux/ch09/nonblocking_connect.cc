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
#include <pthread.h>
#include <iostream>

/**
 * 当对非阻塞的 socket 调用 connect，而连接还没有建立时，connect 的错误码为 errno = EINPORGRESS。
 * 可以使用 select/poll 等函数来监听这个连接失败的 socket 上的可写事件。
 * 当 select/poll 等函数返回后，利用 getsockopt 来读取错误码并清除该 socket 上的错误。
 * 如果 错误码为 0，表示连接成功，否则连接失败。
 * 利用非阻塞的 connect，就可以同时发起多个连接并一起等待。
 * 
 * 非阻塞 connect 的一种实现：
 * 
*/

#define BUFFER_SIZE 1023

int setnoblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 超时连接函数，参数分别是服务器 IP 地址、端口号和超时时间（ms）。
// 函数成功时返回已经处于连接状态的 socket，失败返回 -1.
int unblock_connect(const char* ip, int port, int time) {
    int ret = 0;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fdopt = setnoblocking(sockfd);
    ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == 0) {
        // 若连接成功，则恢复 sockfd 属性并立即返回
        std::cout << "connect with server immediately" << std::endl;
        fcntl(sockfd, F_SETFL, fdopt);
        return sockfd;
    } else if (errno != EINPROGRESS) {
        // 若连接还未建立，只有当 errno = EINPROGRESS 才表示连接还在进行，否则出错返回
        std::cout << "unblock connect not support!" << std::endl;
        return -1;
    } 

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (ret <= 0) {
        // select 超时或者出错，立即返回
        std::cout << "connect timeout" << std::endl;
        close(sockfd);
        return -1;
    }

    if (!FD_ISSET(sockfd, &writefds)) {
        std::cout << "no events on sockfd found" << std::endl;
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    // 调用 getsockopt 开获取并清除 sockfd 上的错误
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        std::cout << "getsockopt failed" << std::endl;
        close(sockfd);
        return -1;
    }

    // 错误号不为 0 表示连接出错
    if (error != 0) {
        std::cout << "connection failed after select with the error = " << error << std::endl;
        close(sockfd);
        return -1;
    }
    // 连接成功
    std::cout << "connection ready after select with the sockfd = " << sockfd << std::endl;
    fcntl(sockfd, F_SETFL, fdopt);
    return sockfd;
}

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = unblock_connect(ip, port, 10);
    if (sockfd < 0) {
        return 1;
    }
    close(sockfd);
    return 0;
}

// 该方法存在移植性问题：
// 1. 非阻塞的 socket 可能导致 connect 始终失败。
// 2. select 对处于 EINPROGRESS 状态下的 socket 可能不起作用。
// 3. 对于出错的 socket, 有些系统（Linux）返回 -1，有些（UNIX）返回 0