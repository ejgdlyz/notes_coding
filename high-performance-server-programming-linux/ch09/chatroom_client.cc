#define _GNU_SOURCE 1
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
#include <poll.h>
#include <iostream>

/**
 * 以 poll 为例，实现一个简单的聊天室程序，说明 IO 复用技术如何同时处理网络连接和用户输入 (比如 ssh)
 * 客户端功能：
    * 1 从标准输入终端读取用户数据，并将用户的数据发送至服务器
    * 2 往标准输出终端打印服务器发送给自己的数据。
 * 
 * 使用 poll 同时监听用户输入和网络连接，并利用 splice 函数将用户的输入内容直接定向到网络连接上发送，
 * 从而实现数据的零拷贝，提高程序的执行效率
*/

#define BUFFER_SIZE 64

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "connection failed, errno = " << errno << ", errstr = " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }

    pollfd fds[2];
    // 注册文件描述符 0 (标准输入) 和 文件描述符 sockfd 上的可读事件
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;
    
    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);

    while (1) {
        ret = poll(fds, 2, -1);
        if (ret < 0) {
            std::cout << "poll failed" << std::endl;
            break;
        }

        if (fds[1].revents & POLLRDHUP) {
            std::cout << "server close the connection" << std::endl;
            break;
        } else if (fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
            std::cout << read_buf << std::endl;
        }

        if (fds[0].revents & POLLIN) {
            // 使用 splice 将用户输入的数据直接写到 sockfd 上（零拷贝）
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);  // 标准输入(fd = 0) -> 流入管道写端 
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);  // 管道读端流出 -> sockfd
        }
    }
    close(sockfd);
    return 0;
}
