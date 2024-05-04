#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <iostream>

/**  统一时间源
 * 信号是一种异步函数（信号的到达时间无法预测），信号处理函数和程序的主循环是两条不同的执行路线。
 * 因此，信号处理函数需要尽可能块地执行完毕，以确保该信号不被屏蔽太久。
 * 统一事件源是一种典型的解决方法：把信号的主要处理逻辑放到程序的主循环中，当信号处理函数被触发时，它只是简单地通知主循环程序接收信号，
 *  并把信号值传递给主循环，主循环在根据接收到的信号值执行模目标信号对应的逻辑代码。
 * 信号处理函数通常使用管道来将信号 “传递”给主循环：信号处理函数往管道的写端写入信号值，主循环则从管道的读端读出该信号值。
 * 主循环如何知道管道上何时有数据可读？使用 IO 复用系统调用来监听管道的读端文件描述符上的可读事件。
 * 这样，信号事件就能和其他 IO 事件一样被处理，即统一事件源。
*/

#define MAX_EVENT_NUMBER 1024
static int pipefd[2];

int setnoblocking(int fd) {
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
    setnoblocking(fd);
}

// 信号处理函数
void sig_handler (int sig) {
    // 保留原始的 errno, 在函数最后恢复 以保证函数的可重入性
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);  // 将信号值写入管道，以通知主循环
    errno = save_errno;
}

// 设置信号的处理函数
void addsig(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;        // 绑定信号处理函数
    sa.sa_flags |= SA_RESTART;          // 重新启动被该信号终止的系统调用
    sigfillset(&sa.sa_mask);            // 设置所有信号
    assert(sigaction(sig, &sa, NULL) != -1);
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
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    // 使用 socketpair 创建管道，注册 pipefd[0] 上的可读事件
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnoblocking(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    // 设置一些信号的处理函数
    addsig(SIGHUP);     // 挂起信号，终端控制进程退出
    addsig(SIGCHLD);    // 子进程结束，发送此信号给父进程，告知父进程子进程已经结束
    addsig(SIGTERM);    // 终止信号，kill 命令的默认信号。
    addsig(SIGINT);     // 用户按下 Ctrl + C，向前台进程组发送 SIGINT 信号
    bool stop_server = false;

    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0) {
            std::cout << "epoll_wait() failure" << std::endl;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                // 就绪文件描述符是 listenfd, 则处理新连接
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
                addfd(epollfd, connfd);
                std::cout << "new connection: connfd = " << connfd << std::endl;
            } else if (sockfd == pipefd[0]) {
                // 就绪文件描述符是 pipefd[0], 则处理信号
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    // 每个信号值占 1 字节，所以按字节来逐个接收信号
                    // 以 SIGTERM 为例，说明如何安全地终止服务器主循环
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i])
                        {
                            case SIGCHLD:
                            case SIGHUP:{
                                continue;
                            }

                            case SIGTERM:
                            case SIGINT: {
                                std::cout << "程序终止" << std::endl;
                                stop_server = true;
                            }
                        }
                    }
                }
            } else {

            }
        }
    }
    std::cout << "close fds" << std::endl;
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    return 0;
}

