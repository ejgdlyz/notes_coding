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
 * 服务器功能是接收客户端数据，并把客户数据发送给每一个登录到该服务上的客户端（发送者自己除外）。
 * 服务器使用 poll 同时监听 socket 和 连接 socket，并使用以空间换时间的策略来提高服务器性能。
 * 
*/

#define USER_LIMIT 5        // 最大用户数量
#define BUFFER_SIZE 64      // 读缓冲区大小
#define FD_LIMIT 65535      // 文件描述符数量限制

// 客户数据: 客户端 socket 地址, 待写到客户端的数据的位置, 从客户端读入的数据
struct client_data {
    sockaddr_in addr;
    char *write_buf;
    char buf[BUFFER_SIZE];
};

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main(int argc, char const *argv[]) {
    // if (argc <= 2) {
    //     std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
    //     return 1;
    // }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    
    int ret = 0;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert( ret != -1);

    // 创建 users 数据， 分配 FD_LIMIT 个 client_data 对象。
    // 每个可能的 socket 连接都可以获得一个这样的对象，并且 socket 的值可以直接用来索引（数组下标） socket 连接对应的 client_data 对象
    // 这是将 socket 和 客户数据关联的简单而高效的方法
    client_data *users = new client_data[FD_LIMIT];

    // 尽管分配了足够多的 client_data 对象，但为了提高 poll 的性能，仍然有必要限制用户的数量
    pollfd fds[USER_LIMIT + 1];
    int user_cnt = 0;
    for (int i = 1; i <= USER_LIMIT; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }

    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (true) {
        // std::cout << "start ..." << std::endl;
        ret = poll(fds, user_cnt + 1, -1);
        if (ret < 0) {
            std::cout << "poll failure!" << std::endl;
            break;
        }

        for (int i = 0; i < user_cnt + 1; ++i) {
            if (fds[i].fd == listenfd && (fds[i].revents & POLLIN)) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);

                if (connfd < 0) {
                    std::cout << "errno = " << errno << " errstr = " << strerror(errno) << std::endl;
                    continue;
                }

                // 如果请求太多，则关闭新到的连接
                if (user_cnt >= USER_LIMIT) {
                    const char* info = "to many users\n";
                    std::cout << info;
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                // 对于新连接，同时修改 fds 和 users 数组
                // users[connfd] 对应于新连接文件描述符 connfd 的客户数据
                ++user_cnt;
                users[connfd].addr = client_addr;
                setnonblocking(connfd);
                fds[user_cnt].fd = connfd;
                fds[user_cnt].events = POLLIN | POLLHUP | POLLERR;
                fds[user_cnt].revents = 0;
                std::cout << "comes a new user, now have " << user_cnt << " users" << std::endl;
            } else if (fds[i].revents & POLLERR) {
                std::cout << "get an error from " << fds[i].fd;
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t len = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0) {
                    std::cout << "get socket option failed!" << std::endl;
                }
                continue;
            } else if (fds[i].revents & POLLHUP) {
                // 如果客户端关闭连接，则服务器也关闭对应的连接，并将用户数 -1
                users[fds[i].fd] = users[fds[user_cnt].fd];
                close(fds[i].fd);
                fds[i] = fds[user_cnt];
                --i;
                --user_cnt;
                std::cout << "a client left" << std::endl;
            } else if (fds[i].revents & POLLIN) {
                int connfd = fds[i].fd;
                memset(users[connfd].buf, 0, BUFFER_SIZE);
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
                std::cout << "Get " << ret << " bytes of client data " << users[connfd].buf << " from " << connfd << std::endl;

                if (ret < 0) {
                    // 操作出错，关闭连接
                    if (errno != EAGAIN) {
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_cnt].fd];
                        fds[i] = fds[user_cnt];
                        --i;
                        --user_cnt;
                    }
                } else if (ret == 0) {

                } else {
                    // 如果接收到客户端数据，则通知其他 sock 连接准备写数据
                    for (int j = 1; j <= user_cnt; ++j) {
                        if (fds[j].fd == connfd) {
                            continue;
                        }
                        // 其他 socket 此刻不能再读（当前 client 继续发多个消息的话，这些消息会阻塞在 socket 的 send buffer）
                        // 将它们设置为仅写，因为如果可以继续读，在当前 client 连续发多个消息的情况下，其他 client 将漏收后续的消息
                        fds[j].events |= ~POLLIN;   // 
                        fds[j].events |= POLLOUT;   // 监听可写事件
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            } else if (fds[i].revents & POLLOUT) {
                int connfd = fds[i].fd;
                if (!users[connfd].write_buf) {
                    continue;
                }
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
                users[connfd].write_buf = NULL;
                // 写完数据后需要重新注册 fds[i] 上的可读事件
                fds[i].events |= ~POLLOUT;      // 
                fds[i].events |= POLLIN;        // 监听可读事件
            }
        }
    }
    delete [] users;
    close(listenfd);
    return 0;
}
