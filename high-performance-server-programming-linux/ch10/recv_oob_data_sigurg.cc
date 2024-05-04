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
#include <iostream>
#include <functional>

/** 网络编程相关的信号
 * 1 SIGHUP 
 *  当挂起进程的控制终端时，SIGHUP 信号将被触发。对于没有控制终端的后台程序而言，它们经常利用 SIGHUP 信号来强制服务器重读配置文件。
 *  ps -ef | grep xinetd
 *  sudo lsof -p pid
 * sudo strace -p pid &> a.txt 跟踪程序执行时调用的系统调用和接收到的信号
 * 
 * 2 SIGPIPE
 *  默认情况下，往一个读端关闭的管道或 socket 连接中写数据将引发 SIGPIPE 信号。需要在代码中捕获该信号，或者至少忽略它，否则程序收到 SIGPIPE 信号
 *  的默认行为是结束进程，而错误的写操作而导致程序退出是不期望的。
 *   2.1 可以使用 send() 的 MSG_NOSIGNAL 标志来近址写操作触发 SIGPIPE 信号，再根据 errno 判断管道或者 socket 连接的读端是否关闭。
 *   2.2 利用 I/O 复用系统调用来检测管道和 socket 连接的读端是否已经关闭。
 *      以 poll 为例，当管道的读端关闭时，写端 fd 上的 POLLHUP 事件将被触发；当 socket 连接关闭时，socket 上的 POLLRDHUP 事件被触发。
 * 
 * 3 SIGURG
 * Linux 下，内核通知应用程序带外数据到达主要有两种方法：
 *  3.1 I/O 复用技术，比如 select 等系统调用收到带外数据时将返回，并向应用程序报告 socket 上的异常事件
 *  3.2 使用 SIGURG 信号，代码如下
 * 
 * 用 SIGURG 检测带外数据是否到达
*/

#define BUF_SIZE 1024
static int connfd;

// SIGURG 信号的处理函数
void sig_urg(int sig) {
    int save_errno = errno;
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    int ret = recv(connfd, buffer, BUF_SIZE - 1, MSG_OOB);  // 接收带外数据
    std::cout << "got " << ret << " bytes of oob data: " << buffer << std::endl;
    errno = save_errno;
}

void addsig(int sig, void (*sig_handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
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

    struct sockaddr_in client_addr;
    socklen_t client_len;
    connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
    assert (connfd >= 0);

    addsig(SIGURG, sig_urg);
    // 使用 SIGURG 之前，必须设置 socket 格桑的宿主进程或者进程组
    fcntl(connfd, F_SETOWN, getpid());

    char buffer[BUF_SIZE];
    // 循环接收普通数据
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        ret = recv(connfd, buffer, BUF_SIZE - 1, 0);
        if (ret <= 0) {
            break;
        }
        std::cout << "got " << ret << " bytes of normal data: " << buffer << std::endl;
    }

    close(connfd);
    close(listenfd);
    return 0;
}

/*
got 11 bytes of normal data: normal_data
got 1 bytes of oob data: a
got 7 bytes of normal data: oob_dat
got 11 bytes of normal data: normal_data`
*/

// p192 带外数据总结： TCP 模块如何发送和接收带外数据
/**
 * 1 发送/接收带外数据：使用带 MSG_OOB 标志的 send/recv 系统调用
 * 2 检测带外数据是否到达的两种方法：I/O 复用系统调用报告的异常事件 和 SIGURG 信号。
 * 3 应用程序进一步判断带外数据在数据流中的位置：sockatmark 系统调用可以判断一个 socket 是否处于带外标记
 *      ，即该 socket 上下一个将被读取到的数据是否是带外数据
 * 
*/