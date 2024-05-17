#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>
#include <functional>

/**
 * 使用共享内存的聊天室服务器程序
 * 将所有客户 socket 连接的都缓冲设计为一块共享内存
*/


#define USER_LIMIT 5
#define BUFFER_SIZE 1024
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define PROCESS_LIMIT 655350


/// 处理一个客户连接必要的数据
struct client_data {
    sockaddr_in address;    /// 客户端 socket 地址
    int connfd;             /// socket 文件描述符
    pid_t pid;              /// 处理该连接的子进程 pid
    int pipefd[2];          /// 和父进程通信的管道
};

static const char* g_shm_name = "/my_shm";
int sig_pipefd[2];
int epollfd;
int listenfd;
int shmfd;
char* share_mem = nullptr;

/// 客户连接数组，进程使用客户连接的编号来索引该数组，即可取得相关的客户端连接数据
client_data* g_users = nullptr;

/// 子进程和客户连接的映射关系表。使用进程 pid 来索引该数组，即可取得该进程所处理的客户端连接的编号
int* sub_process = nullptr;

/// 当前客户数量
int g_user_count = 0;
bool stop_child = false;

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
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig, void(*handler)(int), bool restart = true) {
    struct sigaction sa;
    // bzero(&sa, sizeof(sa));
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void del_resource() {
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(g_shm_name);
    delete[] g_users;
    delete[] sub_process;
}

/// 终止一个子进程
void child_term_handler (int sig) {
    stop_child = true;
}

/// 子进程运行的函数
/**
 * idx 指出子进程处理的客户连接的编号
 * g_users 保存所有客户连接数据数组
 * share_mem 指向共享内存起始地址
*/
int run_child(int idx, client_data* g_users, char* share_mem) {
    epoll_event events[MAX_EVENT_NUMBER];

    /// 子进程使用 IO 复用技术来同时监听两个文件描述符：客户连接 socket、与父进程通信的管道文件描述符
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    int connfd = g_users[idx].connfd;
    addfd(child_epollfd, connfd);
    int pipefd = g_users[idx].pipefd[1];  /// 管道写
    addfd(child_epollfd, pipefd);
    int ret;
    
    /// 子进程需要设置自己的信号处理函数
    addsig(SIGTERM, child_term_handler, false);

    while (!stop_child) {
        int number = epoll_wait(child_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            std::cout << "child epoll failure" << std::endl;
            break;
        } 

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;

            /// 此子进程负责的客户连接有数据到达
            if ((sockfd == connfd) && (events[i].events & EPOLLIN)) {
                memset(share_mem + idx * BUFFER_SIZE, '\0', BUFFER_SIZE);

                /// 将客户数据读取到对应的读缓存中。
                /// 该缓存是共享内存的一段，它开始于 idx * BUFFER_SIZE 处，长度为 BUFFER_SIZE 字节
                /// 所以，每个客户连接的读缓存是共享的
                ret = recv(connfd, share_mem + idx * BUFFER_SIZE, BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        stop_child = true;
                    }
                } else if (ret == 0) {
                    stop_child = true;
                } else {
                    /// 成功读取客户数据后就通知主进程（通过管道）来处理
                    send(pipefd, reinterpret_cast<char*>(&idx), sizeof(idx), 0);
                }
            } else if ((sockfd == pipefd) && (events[i].events | EPOLLIN)) {
                /// 主进程通知此进程（通过管道）将第 client 个客户端的数据发送到本进程负责的客户端
                
                int client = 0;

                /// 接收主进程发送来的数据，即 有客户数据到达的连接的编号
                ret = recv(sockfd, reinterpret_cast<char*>(&client), sizeof(client), 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        stop_child = true;
                    } 
                } else if (ret == 0) {
                    stop_child = true;
                } else {
                    send(connfd, share_mem + client * BUFFER_SIZE, BUFFER_SIZE, 0);
                }
            } else {
                continue;
            }
        }
    }
    close(connfd);
    close(pipefd);
    close(child_epollfd);
    return 0;
}

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address));
    assert(ret != -1);
    
    ret = listen(listenfd, 5);
    assert(ret != -1);
    
    g_user_count = 0;
    g_users = new client_data[USER_LIMIT + 1];
    sub_process = new int[PROCESS_LIMIT];
    for (int i = 0; i < PROCESS_LIMIT; ++i) {
        sub_process[i] = -1;
    }

    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(epollfd, sig_pipefd[0]);

    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);

    bool stop_server = false;
    bool terminate = false;
    
    /// 创建共享内存，作为所有客户 socket 连接的读缓存
    shmfd = shm_open(g_shm_name, O_CREAT | O_RDWR, 0666);
    assert(shmfd != -1);
    ret = ftruncate(shmfd, USER_LIMIT * BUFFER_SIZE); /// 设置共享内存的大小
    assert(ret != -1);

    /// 调用 mmap 将共享内存映射到当前进程的地址空间，返回映射区的首地址
    share_mem = (char*)mmap(NULL, USER_LIMIT * BUFFER_SIZE
                            , PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    assert(share_mem != MAP_FAILED);
    /// 已经将共享内存映射到了当前进程地址空间，关闭共享内存的文件描述符
    close(shmfd); 

    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            std::cout << "parent epoll failture" << std::endl;
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;

            if (sockfd == listenfd) {
                /// 新的客户连接到来
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(listenfd
                                    , reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

                if (connfd < 0) {
                    std::cout << "errno=" << errno << ", errstr=" << strerror(errno) << std::endl;
                    continue;
                }

                if (g_user_count >= USER_LIMIT) {
                    const char* info = "too many users\n";
                    std::cout << info;
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                /// 保存第 g_user_count 个客户连接的相关数据
                g_users[g_user_count].address = client_addr;
                g_users[g_user_count].connfd = connfd;
                /// 在主进程和子进程间建立管道，以传递必要的数据
                ret = socketpair(PF_UNIX, SOCK_STREAM, 0, g_users[g_user_count].pipefd);
                assert(ret != -1);

                pid_t pid = fork();
                if (pid < 0) {
                    close(connfd);
                    continue;
                } else if (pid == 0) {
                    /// 子进程
                    close(epollfd);
                    close(listenfd);
                    close(g_users[g_user_count].pipefd[0]);
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(g_user_count, g_users, share_mem);
                    munmap(reinterpret_cast<void*>(share_mem), USER_LIMIT * BUFFER_SIZE);
                    exit(0);
                } else {
                    /// 父进程
                    close(connfd);
                    close(g_users[g_user_count].pipefd[1]);
                    addfd(epollfd, g_users[g_user_count].pipefd[0]);
                    g_users[g_user_count].pid = pid;

                    /// 记录新的客户连接在数组 g_users 中的索引值，建立进程 pid 和该索引之间的映射关系
                    sub_process[pid] = g_user_count;
                    ++g_user_count;
                }
            } else if (sockfd == sig_pipefd[0] && events[i].events | EPOLLIN) {
                /// 处理信号事件
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i])
                        {
                        case SIGCHLD:       /// 子进程退出，表示某个客户端关闭了连接
                        {
                            pid_t pid;
                            int stat;
                            while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                /// 用子进程的 pid 取得被关闭的客户连接的编号
                                int del_user = sub_process[pid];
                                sub_process[pid] = -1;
                                if (del_user < 0 || del_user > USER_LIMIT) {
                                    continue;
                                }

                                /// 清除第 del_user 个客户连接使用的相关数据
                                epoll_ctl(epollfd, EPOLL_CTL_DEL, g_users[del_user].pipefd[0], 0);
                                close(g_users[del_user].pipefd[0]);
                                g_users[del_user] = g_users[--g_user_count];
                                sub_process[g_users[del_user].pid] = del_user;
                            }
                            if (terminate && g_user_count == 0) {
                                stop_server = true;
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            /// 结束服务器程序
                            std::cout << "kill all the child now" << std::endl;
                            if (g_user_count == 0) {
                                stop_server = true;
                                break;
                            }
                            for(int i = 0; i < g_user_count; ++i) {
                                int pid = g_users[i].pid;
                                kill(pid, SIGTERM);
                            }
                            terminate = true;
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                /// 某个子进程想父进程写入了数据
                int child = 0;
                /// 读取管道数据，child 变量记录是哪个客户连接有数据到达
                ret = recv(sockfd, reinterpret_cast<char*>(&child), sizeof(child), 0);
                std::cout << "read data from child accross pipe" << std::endl;
                if (ret == -1) {
                    continue;
                } else if(ret == 0) {
                    continue;
                } else {
                    /// 向负责处理第 child 个客户连接的子进程之外的其他进程发送消息，通知他们有客户数据需要写

                    for (int j = 0; j < g_user_count; ++j) {
                        if (g_users[j].pipefd[0] != sockfd) {
                            std::cout << "send data to child accorss pipe" << std::endl;
                            send(g_users[j].pipefd[0], reinterpret_cast<char*>(&child), sizeof(child), 0);
                        }
                    }
                }
            }
        }
    }
    del_resource();
    return 0;
}



/* 注意
    1. 编译该代码需要链接库 -lrt (real time)
    2. 尽管使用了读缓存，但是每个子进程都只向自己所处理的客户连接所对应的那部分读缓存写数据，
        所以使用共享内存的目的只是为了共享读。每次子进程在使用共享内存的时候无需加锁。
    3. 服务器程序在启动的时候给数组 g_users 分配了足够多的空间，十七可以存储所有可能的客户连接的相关数据。
        同样，一次性给数组 sub_process 分配的空间也足以存储所有可能的子进程相关数据。牺牲空间换时间。
*/

