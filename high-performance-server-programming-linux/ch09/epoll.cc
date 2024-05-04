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
 * LT 和 ET 模式 
 * LT: 水平触发模式，默认。
 * ET: 向 epoll 内核事件表注册一个文件描述符上的 EPOLLET 事件，epoll 将以 ET 模式来操作该文件描述符。更加高效。
 * 对于 LT 工作模式的文件描述符，当 epoll_wait 检测到其上有事件发生并将此事件通知应用程序后，应用程序可以不立即处理该事件。
 *      这样，当应用程序下一次调用 epoll_wait 时，epoll_wait 还会再次向应用程序通告此事件，直到该事件被处理。
 * 对于 ET 工作模式的文件描述符，当 epoll_wait 检测到其上有事件发生并将此事件通知应用程序后，应用程序必须立即处理该事件，
 *      因为后续的 epoll_wait 调用将被不在向应用程序通知该事件。
 * ET 降低了同一个 epoll 事件被重复触发的次数，效率比 LT 模式高。
 * 
 * 1 int epollfd = epoll_create(5); // 通过 epollfd 指向一个内核事件表
 * 2 epoll_ctl(epollfd, int op, itn fd, struct epoll_evnet* event);  // 通过 op 指示注册、修改、删除 fd 上的事件
    * struct epoll_event
    *  {
        uint32_t events;  // Epoll events
        epoll_data_t data;    // User data variable, epoll_data_t 是一个联合体，其成员 fd 和 data.ptr 不能同时使用。(data.ptr 可指向自定义数据结构，包含fd即可)
    *   } __EPOLL_PACKED;
* 3 int epoll_wait(int epollfd, struct epoll_evnet* event, int maxevents, int timeout); 
*/

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 5

// 将文件描述符设置为非阻塞的
int setnoblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将文件描述符 fd 上的 EPOLLIN 注册到 epollfd 指示的 epoll 内核事件表中，
// 参数 enable_et 指定是否对 fd 启用 ET 模式
void addfd(int epollfd, int fd, bool enable_et) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnoblocking(fd);
}

// LT 模式工作流程
void lt(epoll_event* events, int number, int epollfd, int listenfd) {
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i) {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd) {
            sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
            addfd(epollfd, connfd, false);  // 将 connfd 注册到 epollfd, 并禁用 ET 模式
        } else if (events[i].events & EPOLLIN) {
            // 只要 socket 读缓存中还有未读出的数据，这段代码就会被触发
            std::cout << "event trigger once" << std::endl;
            memset(buf, 0, BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if (ret <= 0) {
                close(sockfd);
                continue;
            }
            std::cout << "get " << ret << " bytes of content: " << buf << std::endl;    

        } else {
            std::cout << "something else happened" << std::endl;
        }
    }
}

// ET 模式的工作流程
void et(epoll_event* events, int number, int epollfd, int listenfd) {
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i) {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd) {
            sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
            addfd(epollfd, connfd, true);  // 将 connfd 注册到 epollfd, 并启用 ET 模式
        } else if (events[i].events & EPOLLIN) {
            // 这段代码不会重复触发，所以需要循环读取数据，以确保把 socket 读缓存中所有数据读出
            std::cout << "event trigger once" << std::endl;
            while (1) {
                memset(buf, 0, BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    // 对于非阻塞 IO，下面的条件成立表示数据已经全部读取完毕。
                    // 之后，epoll 就能再次触发 sockfd 上的 EPOLLIN 事件，以驱动下一次读操作
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        std::cout << "read later" << std::endl;
                        break;
                    }
                    close(sockfd);
                    break;
                } else if (ret == 0) {
                    close(sockfd);
                } else {
                    std::cout << "get " << ret << " bytes of content: " << buf << std::endl;    
                }
            }
        } else {
            std::cout << "something else happened" << std::endl;
        }
    }
}


int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
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
    assert(ret != -1);    
    
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);      // 5 只是给内核一个提示，告诉它事件表多大，现在不起作用
    assert(epollfd != -1);
    addfd(epollfd, listenfd, true);

    while (1) {
        // 一段超时时间内等待一组文件描述符上的事件.
        // 成功时返回就绪的文件描述符个数, 失败返回 -1 并设置 errno
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);  
        if (ret < 0) {
            std::cout << "epoll failure: errno =" << errno << ", errstr = " << strerror(errno) << std::endl;
            break;
        }

        lt(events, ret, epollfd, listenfd);  // 使用 LT 模式
        // et(events, ret, epollfd, listenfd);  // 使用 ET 模式
        // 每个 ET 模式的文件描述符都应该是非阻塞的。如果文件描述符是阻塞的，那么读或写操作将会因为没有后续的事件一致处于阻塞状态。

    }
    close(listenfd);
    return 0;
}

/* 结果

lambda@lambda-virtual-machine:~/notes$ telnet 127.0.0.1 9725
qwertyuiopasdfghjkl

LT 
lambda@lambda-virtual-machine:~/notes/high_performance_linux_server_programming/tests$ ./test_epoll 127.0.0.1 9725
event trigger once
get 4 bytes of content: qwer
event trigger once
get 4 bytes of content: tyui
event trigger once
get 4 bytes of content: opas
event trigger once
get 4 bytes of content: dfgh
event trigger once
get 4 bytes of content: jkl
event trigger once
get 1 bytes of content: 


ET
lambda@lambda-virtual-machine:~/notes/high_performance_linux_server_programming/tests$ ./test_epoll 127.0.0.1 9725
event trigger once
get 4 bytes of content: qwer
get 4 bytes of content: tyui
get 4 bytes of content: opas
get 4 bytes of content: dfgh
get 4 bytes of content: jkl
get 1 bytes of content: 


LT 下只要缓冲区中还有数据，EPOLLIN 事件就会被触发，所以输出了 多次 "event trigger once" .
ET "event trigger once" 只输出一次，"event trigger once"，事件只在数据状态变为可读时发生了一次，即使在数据被分多次读取的过程中仍有数据可读。
*/
