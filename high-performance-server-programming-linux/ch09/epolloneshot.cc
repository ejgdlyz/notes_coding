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
 * 即使是 ET 模式，一个 socket 上的事件还是可能被触发多次。这在并发程序中引起一个问题：
 * 比如一个线程在读物完某个 socket 上的数据后开始处理这些数据，而在数据的处理过程中该 socket 上又有新的数据可读（EPOLLIN再次被触发），此时
 * 另外一个线程被唤醒来读取这些新的数据，于是就出现了两个线程同时操作一个 socket 局面。而期望的是一个 socket 连接在任意时刻都只被一个线程处理。
 * 这可以通过 EPOLLONESHOT 事件实现。
 * 
 * 对于注册了 EPOLLONESHOT 事件的文件描述符，操作系统最多触发其上注册的 一个 可读、可写或者异常事件，且仅触发一次，
 * 除非使用 epoll_ctl 函数重置该文件描述符上注册的 EPOLLONESHOT 事件。
 * 这样，当一个线程在处理某个 socket 时，其他线程不会操作该 socket。这也说明一个线程处理完了 注册 EPOLLONESHOT 事件的某个 socket, 
 *  该线程就应该立即重置 socket 上的 EPOLLONESHOT 事件，确保这个 socket 下一次可读时，
 *  其 EPOLLIN 事件能被触发，进而让其他线程有机会继续处理这个 socket.
 * 
 * 使用 EPOLLONESHOT 事件
*/

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 5
struct fds {
    int epollfd;
    int sockfd;
};

// 将文件描述符设置为非阻塞的
int setnoblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将文件描述符 fd 上的 EPOLLIN 注册到 epollfd 指示的 epoll 内核事件表中，
// 参数 oneshot 指定是否注册 fd 上的 EPOLLONESHOT 事件
void addfd(int epollfd, int fd, bool oneshot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnoblocking(fd);
}

// 重置 fd 上的事件，这样操作之后，尽管 fd 上的 EPOLLONESHT 事件被注册，但是操作系统仍然会触发 fd 上的 EPOLLIN 事件，且只触发一次
void reset_oneshot(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 工作线程
void* worker(void* arg) {
    int sockfd = ((fds*) arg)->sockfd;
    int epollfd = ((fds*) arg)->epollfd;
    std::cout << "start new thread to receive data on fd = " << sockfd << std::endl;
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);
    
    // 循环读取 sockfd 上的数据，直到遇到 EAGAIN 错误
    std::string str;
    while(1) {
        memset(buf, 0, BUFFER_SIZE);
        int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
        if (ret == 0) {
            close(sockfd);
            std::cout << "foreiner closed the connection" << std::endl;
            break;
        } else if (ret < 0) {
            if (errno == EAGAIN) {
                reset_oneshot(epollfd, sockfd);  // 重置 EPOLLONESHOT 事件
                std::cout << "read later" << std::endl;
                break;
            }
        } else {
            str += std::string(buf);
            // std::cout << "get " << ret << " bytes of content: " << buf << std::endl;
            // 休眠 5s，模拟数据处理过程
            sleep(5);
        }
    }
    std::cout << "fd = " << sockfd << ", content = " << str << std::endl;
    std::cout << "end thread receiving data on fd = " << sockfd << std::endl;
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
    // 监听 socket listenfd 上不能注册 EPOLLONESHOT 事件，否则应用程序只能处理一个客户连接。
    // 因为后续的客户连接请求将不在触发 listenfd 上的 EPOLLIN 事件
    addfd(epollfd, listenfd, false);

    while (1) {
        // 一段超时时间内等待一组文件描述符上的事件.
        // 成功时返回就绪的文件描述符个数, 失败返回 -1 并设置 errno
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);  
        if (ret < 0) {
            std::cout << "epoll failure: errno =" << errno << ", errstr = " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < ret; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);

                // 对每非监听文件描述符都注册 EPOLLONESHOT 事件
                addfd(epollfd, connfd, true);
            } else if (events[i].events | EPOLLIN) {
                pthread_t thread;
                fds fds_for_new_worker;
                fds_for_new_worker.epollfd = epollfd;
                fds_for_new_worker.sockfd = sockfd;
                // 新启动一个工作线程为 sockfd 服务
                pthread_create(&thread, NULL, worker, (void*)&fds_for_new_worker);
            } else {
                std::cout << "something else happened" << std::endl;
            }
        }
    }
    close(listenfd);
    return 0;
}

/**
 * 如果一个工作线程处理完某个 socket 上的一次请求（休眠5s)之后，又接收到该 socket 上新的客户请求，则该线程将继续为这个 socket 服务。
 * 由于该 socket 上注册了 EPOLLONESHOT 事件，其他线程没有机会接触这个 socket。
 * 如果工作线程等待 5s 仍然没有收到该 socket 上的下一批客户数据，则它将放弃为该 socket 服务。调用 reset_oneshot 来重置该 socket 上
 *      注册的事件，使得 epoll 有机会再次检测到该 socket 上的 EPOLLIN 事件，使得其他线程可以为该 socket 提供服务。
 * 
 * 尽管一个 socket 在不同的事件可能被不同的线程处理，但是同一时刻肯定只有一个线程为它服务，从而保证了连接的完整性。 
 * 
*/

/*  结果 
lambda@lambda-virtual-machine:~/notes$ telnet 127.0.0.1 9700  // 只发字母
qwertyuiop  // 1
qwertyuiop  // 3

lambda@lambda-virtual-machine:~/notes$ telnet 127.0.0.1 9700  // 只发数字
1234567890  // 2

// 关闭 EOPLLONESHOT
lambda@lambda-virtual-machine:~/notes/high_performance_linux_server_programming/tests$ ./test_epolloneshot 127.0.0.1 9700
start new thread to receive data on fd = 5
start new thread to receive data on fd = 6
start new thread to receive data on fd = 5
read later
fd = 5, content = qwerop
tyui
end thread receiving data on fd = 5
read later
fd = 6, content = 1234567890

end thread receiving data on fd = 6
read later
fd = 5, content = tyuiqwerop

end thread receiving data on fd = 5

// 打开 EOPLLONESHOT
lambda@lambda-virtual-machine:~/notes/high_performance_linux_server_programming/tests$ ./test_epolloneshot 127.0.0.1 9700
start new thread to receive data on fd = 5
start new thread to receive data on fd = 6
read later
fd = 6, content = 1234567890

end thread receiving data on fd = 6
read later
fd = 5, content = qwertyuiop
qwertyuiop

end thread receiving data on fd = 5

*/
