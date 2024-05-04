#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

/*
    基于 epoll 的回声服务端
    边缘触发 ：
        通过 errno 变量验证 read 函数全部读取
        调整 read / write 函数 以 非阻塞方式工作，不然会引起服务器的长时间停顿
    
    客户端发送多少次数据，服务端相应地产生多少事件
*/

#define BUF_SIZE 4  // 减小缓冲区大小，防止服务器一次性读取接收的数据
#define EPOLL_SIZE 50
void error_handling (char *message);
void setnonblockingmode(int fd);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t adr_sz;
    int str_len, i;
    char buf[BUF_SIZE];

    struct epoll_event *ep_events;  // 保存发生事件的文件描述符集合
    struct epoll_event event;
    int epfd, event_cnt;

    serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handling("bind() error!");
    }

    if(listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error!");
    }

    epfd = epoll_create(EPOLL_SIZE);  // 创建 epoll 例程, 区分不同的 epoll 例程, EPOLL_SIZE 仅供 OS 参考
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);  // 动态分配  

    event.events = EPOLLIN;  // 需要读取数据情况
    event.data.fd = serv_sock;  // 
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);  // 注册监视对象文件描述符

    while(1)
    {
        // epfd: 事件发生监视范围的 epoll 例程的文件描述符, 发生事件的文件描述符集合, 最大事件数, 等待时间, -1 表示一直等待该事件发生
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);  
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }
        
        puts("return epoll_wait");  // epoll_wait 调用次数

        for (i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                adr_sz = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &adr_sz);
                
                setnonblockingmode(clnt_sock);  // 创建的套接字改为非阻塞模式
                event.events = EPOLLIN | EPOLLET;  // 边缘触发注册方式

                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);  // 在 epoll例程 epfd 中注册客户端文件描述符, 以监视 event 中的事件
                printf("connected client : %d \n", clnt_sock);
            }
            else
            {
                while(1)  // 边缘触发方式中，发生事件时需要读取输入缓冲中的所有数据，因此需要循环调用 read
                {
                    str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);  // 循环调用 read 函数 以读取全部数据
                    if (str_len == 0)  // 关闭连接
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);  // 从例程 epfd 中删除 客户端文件描述符 
                        close(ep_events[i].data.fd);
                        printf("closed client: %d\n", ep_events[i].data.fd);
                    }
                    else if(str_len < 0)  // read 返回 -1 且 errno 为 EAGAIN 表示 读取了全部数据
                    {
                        if (errno == EAGAIN)
                            break;
                    }
                    else
                    {
                        write(ep_events[i].data.fd, buf, str_len);  // echo
                    }
                }
                
            }
        }
    }

    close(serv_sock);
    close(epfd);
    free(ep_events);  // 释放动态分配的内存
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void setnonblockingmode(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);  // 传递 F_GETFL，获取第一个参数所指的文件描述符属性(int)
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);  // 传递 F_SETFL， 可以更改文件描述符。在此基础上, 添加非阻塞 O_NONBLOCK 标志
}