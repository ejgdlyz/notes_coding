/**
 * 基于 epoll 实现 Reactor 
 * https://aceld.gitbooks.io/libevent/content/32_epollde_fan_ying_dui_mo_shi_shi_xian.html 
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <functional>

#define MAX_EVENTS  1024
#define MAX_BUF_LEN 128
#define SERV_PORT   8080

typedef std::function<void(int fd, int events, void *arg)> CallBack;

struct Event
{
    int fd;                     // cfd/listenfd
    int events;                 // EPOLLIN/EPOLLOUT
    void *arg;                  // 指向自己的结构体指针
    CallBack call_back;         // 回调函数
    int status;                 // 1: 在监听事件中，0: 不在
    char buf[MAX_BUF_LEN];
    int len;
    time_t lastActive;          // 最后一次响应时间, for timeout 
};

// epoll_create() 返回的句柄
int g_efd;
// g_events[MAX_EVENTS] 用于 listen fd
struct Event g_events[MAX_EVENTS + 1];

void SetEvent(Event *ev, int fd, CallBack cb, void *arg)
{
    ev->fd = fd;
    ev->call_back = cb;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    ev->lastActive = time(nullptr);

    return;
}

void RecvData(int fd, int events, void *arg);
void SendData(int fd, int events, void *arg);

void AddEvent(int efd, int events, Event *ev)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;
    epv.events = ev->events = events;

    if (ev->status == 1)
    {
        op = EPOLL_CTL_MOD;
    }
    else 
    {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if (epoll_ctl(efd, op, ev->fd, &epv) < 0)
    {
        std::cout << "event adding failure: fd=[" << ev->fd << "], events=[" << events << "]" << std::endl;
    }
    else
    {
        std::cout << "event adding success: fd=[" << ev->fd << "], op=[" << op << "], events=[" << events << "]" << std::endl;
    }
}

void DelEvent(int efd, Event *ev) 
{
    struct epoll_event epv = {0, {0}};

    if (ev->status != 1)
    {
        return;
    }

    epv.data.ptr = ev;
    ev->status = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);
}

void AcceptConnection(int listenfd, int events, void *arg)
{
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    int connFd = accept(listenfd, (struct sockaddr *)(&clientAddr), &len);
    if (connFd == -1)
    {
        if (errno != EAGAIN || errno != EINTR)
        {
            // error process
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return;
    }

    int i;
    do
    {
        for (i = 0; i < MAX_EVENTS; ++i)
        {
            if (g_events[i].status == 0)
            {
                // 复用
                break;
            }
        }

        if (i == MAX_EVENTS)
        {
            printf("%s: max connection limit, MaxConnNums=[%d]\n", __func__, MAX_EVENTS);
            break;
        }

        int flags = fcntl(connFd, F_SETFL, O_NONBLOCK);
        if (flags < 0)
        {
            printf("%s: fcntl nonblocking failure, %s\n", __func__, strerror(errno));
            break;
        }

        SetEvent(&g_events[i], connFd, RecvData, &g_events[i]);
        AddEvent(g_efd, EPOLLIN, &g_events[i]);
    } while (0);
    
    printf("new connect [%s:%d][time:%ld], pos[%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), g_events[i].lastActive, i);
}

void RecvData(int fd, int events, void *arg)
{
    Event *ev = reinterpret_cast<Event *>(arg);
    int len;

    len = recv(fd, ev->buf, sizeof(ev->buf), 0);
    DelEvent(g_efd, ev);

    if (len > 0)
    {
        ev->len = len;
        ev->buf[len] = '\0';
        printf("Client=[%d]: %s\n", fd, ev->buf);

        // 转为发送事件
        SetEvent(ev, fd, SendData, ev);
        AddEvent(g_efd, EPOLLOUT, ev);
    }
    else if (len == 0)
    {
        close(ev->fd);
        // ev - g_events 地址相减得到偏移位置
        printf("fd=[%d], pos=[%d], closed.\n", fd, (int)(ev - g_events));
    }
    else 
    {
        close(ev->fd);
        printf("recv error, fd=[%d], errno=[%d], errstr=[%s]\n", fd, errno, strerror(errno));
    }
}

void SendData(int fd, int events, void *arg)
{
    Event *ev = reinterpret_cast<Event *>(arg);
    int len;

    len = send(fd, ev->buf, ev->len, 0);

    DelEvent(g_efd, ev);
    
    if (len > 0)
    {
        printf("send data: fd=[%d], len=[%d], data=[%s]\n", fd, len, ev->buf);
        SetEvent(ev, fd, RecvData, ev);
        AddEvent(g_efd, EPOLLIN, ev);
    }
    else 
    {
        close(ev->fd);
        printf("send data error: fd=[%d], errstr=[%s]\n", fd, strerror(errno));
    }
}

void InitListenSocket(int efd, short port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

    SetEvent(&g_events[MAX_EVENTS], listenfd, AcceptConnection, &g_events[MAX_EVENTS]);
    AddEvent(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);

    struct sockaddr_in serv_addr;

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    int iRet = bind(listenfd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr));
    if (iRet != 0)
    {
        perror("bind error");
    }

    iRet = listen(listenfd, 20);
    if (iRet != 0)
    {
        perror("listen error");
    }
}

int main(int argc, char const *argv[])
{
    unsigned short port = SERV_PORT;

    if (argc == 2)
    {
        port = atoi(argv[1]);
    }

    g_efd = epoll_create(MAX_EVENTS + 1);

    if (g_efd <= 0)
    {
        printf("create_create error: %s, errstr=[%s]\n", __func__, strerror(errno));
        exit(0);
    }

    // 初始化 listenfd 并将其包装为事件
    InitListenSocket(g_efd, port);

    // 事件循环
    epoll_event events[MAX_EVENTS + 1];

    printf("server running: port=[%d]\n", port);

    int checkpos = 0, i;

    while (true)
    {
        // 超时验证，每次测试 100 个连接，不测试 listenfd. 当客户端 60s 内没有和服务器通信，则断开该客户段的链接.
        time_t now = time(nullptr);

        for (i = 0; i < 100; ++i)
        {
            if (checkpos == MAX_EVENTS)
            {
                checkpos = 0;
            }

            if (g_events[checkpos].status != 1)
            {
                continue;
            }

            time_t duration = now - g_events[checkpos].lastActive;
            if (duration >= 60)
            {
                close(g_events[checkpos].fd);
                printf("client fd=[%d] timeout\n", g_events[checkpos].fd);
                DelEvent(g_efd, &g_events[checkpos]);
            }
        }

        // 等待事件发生
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
        
        if (nfd < 0)
        {
            printf("epoll_wait error, exit\n");
            break;
        }

        for (i = 0; i < nfd; ++i)
        {
            Event *ev = reinterpret_cast<Event *>(events[i].data.ptr);

            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
            {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
            {
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }

    // 退出前释放所有资源

    return 0;
}