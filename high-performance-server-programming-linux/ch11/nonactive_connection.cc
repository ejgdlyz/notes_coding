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
#include <sys/epoll.h>
#include <pthread.h>
#include "timer_list.h"

/**
 * 基于升序双向链表处理非活动的连接
 * 使用 alarm() 函数周期性地触发 SIGALRM 信号，该信号的信号处理函数利用管道通知主循环执行定时器链表上的定时任务——关闭非活动的连接。
*/

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 5

static int pipefd[2];
// 升序双向链表来管理定时器
static SortedTimerLst timer_lst;
static int epollfd = 0;

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

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void timer_handler() {
    // 定时器处理任务，即调用 tick() 
    timer_lst.tick();
    // 一次 alarm 调用只会引起一次 SIGALRM 信号，所以需要重新定时，以不断触发 SIGALRM 信号
    alarm(TIMESLOT);    // 指定 TIMESLOT 时间后发送 SIGALRM 信号到进程 
}

// 定时器回调函数，它删除非活动连接 socket 上的注册事件，并关闭之
void cb_func(ClientData* user_data) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    std::cout << "close fd = " << user_data->sockfd << std::endl;
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
    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert (listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);  // 创建双向管道，即这对文件描述符可读可写 
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    // 设置信号处理函数
    addsig(SIGALRM);
    addsig(SIGTERM);
    bool stop_server = false;

    ClientData* users = new ClientData[FD_LIMIT];
    std::shared_ptr<ClientData> shared_users(users, [](ClientData* ptr){ delete[] ptr;});
    bool timeout = false;
    alarm(TIMESLOT);  // 设置定时器，在 TIMESLOT 秒后将首次触发 SIGALRM 信号

    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        // epoll_wait() 时收到一个信号就会暂停执行 epoll_wait()，转而执行信号处理函数。
        // 从信号处理函数返回后，epoll_wait 不会继续等待，而是直接返回 -1 并设置 errno = EINTR 表示被信号中断
        if ((number < 0) && (errno != EINTR)) { 
            std::cout << "epoll failure, errno = " << errno << ", errstr = " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            // 处理新到的客户端连接
            if (sockfd == listenfd) {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
                
                addfd(epollfd, connfd);
                users[connfd].address = client_addr;
                users[connfd].sockfd = connfd;

                // 创建定时器，设置其回调函数和超时时间，然后绑定定时器与用户数据，最后将定时器添加到链表 timer_lst 中
                UtilTimer* timer = new UtilTimer;
                timer->user_data = &users[connfd];
                // timer->cb_func = cb_func;
                timer->cb_func = [epollfd](ClientData* user_data){
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
                    assert(user_data);
                    close(user_data->sockfd);
                    std::cout << "close fd = " << user_data->sockfd << std::endl;
                };

                time_t cur = time(0);
                timer->expire = cur + 3 * TIMESLOT;
                users[connfd].timer = timer;
                timer_lst.addTimer(timer);
            } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                // 处理信号, 统一事件源
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    // 处理错误
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                            case SIGALRM: {
                                // 使用 timeout 标记有定时任务需要处理，但不立即处理定时任务，
                                // 因为定时任务的优先级不高，优先处理其他更重要的任务
                                timeout = true;
                                break;
                            }
                            case SIGTERM: {
                                stop_server = true;
                                break;
                            }
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                // 处理客户端连接上接收到的数据
                memset(users[sockfd].buf, 0, BUFFER_SIZE);
                ret = recv(sockfd, users[sockfd].buf, BUFFER_SIZE - 1, 0);
                std::cout << "get " << ret << " bytes of client data: " << users[sockfd].buf 
                        << "from fd = "<< sockfd << std::endl;

                UtilTimer* timer = users[sockfd].timer;
                if (ret < 0) {
                    // 如果发生读错误，则关闭连接，并移除其对应的定时器
                    if (errno != EAGAIN) {
                        cb_func(&users[sockfd]);
                        if (timer) {
                            timer_lst.delTimer(timer);
                        }
                    }
                } else if (ret == 0) {
                    // 如果对方已经关闭连接，则服务器也关闭连接，并移除相应的定时器
                    cb_func(&users[sockfd]);
                    if (timer) {
                        timer_lst.delTimer(timer);
                    }
                } else {
                    // 如果某个客户端上有数据可读，则需要调整该连接对应的定时器，以延迟该连接被关闭的时间
                    if (timer) {
                        time_t cur = time(0);
                        timer->expire = cur + 3 * TIMESLOT;
                        std::cout << "adjust timer once" << std::endl;
                        timer_lst.adjustTimer(timer);
                    }
                }
            } else {
                // others
            }
        }

        // 最后处理定时事件，因为 I/O 事件有更高的优先级
        // 当然，这将导致定时任务不能精确地按照预期的时间执行
        if (timeout) {
            timer_handler();
            timeout = false;
        }
    }
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[2]);
    // delete[] users;

    return 0;
}
