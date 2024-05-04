#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/*
    I/O复用服务端
*/

#define BUF_SIZE 30
void error_handling(char *message);

int main(int argc, char const *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    fd_set reads, cpy_reads;

    socklen_t adr_sz;
    int fd_max, str_len, fd_num, i;
    char buf[BUF_SIZE];
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error");
    }

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);  // 注册服务端套接字
    fd_max = serv_sock;
    printf("serv_sock = %d\n", serv_sock);

    while(1)
    {
        cpy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;

        if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
        {
            break;
        }
        
        if (fd_num == 0)  // 超时
        {
            continue;
        }

        for (i = 0; i < fd_max + 1; ++i)  // 查找发生状态变化的文件描述符
        {
            if(FD_ISSET(i, &cpy_reads))
            {
                if (i == serv_sock)  // 如果为serv_sock，表示受理新的连接请求
                {
                    adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                    
                    FD_SET(clnt_sock, &reads);  // 注册与客户端连接的套接字文件描述符
                    
                    if(fd_max < clnt_sock)
                    {
                        fd_max = clnt_sock;
                    }
                    printf("connected client: %d \n", clnt_sock);
                }
                else  // 非服务端套接字，即要 接收数据 但是需要区分数据是字符串还是断开的EOF
                {
                    str_len = read(i, buf, BUF_SIZE);
                    if (str_len == 0) // 断开连接
                    {
                        FD_CLR(i, &reads);
                        close(i);
                        printf("closed client: %d \n", i);
                    }
                    else
                    {
                        write(i, buf, str_len);  // echo
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}

void error_handling(char * message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}