#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handing(char* message);

int main(int argc, char const *argv[])
{
    // 客户端套接字
    int sock;

    // 结构体 保存ip地址与端口等信息
    struct sockaddr_in serv_addr;
    char message[30];
    int str_len;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        error_handing("socket() error!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);  // 这里不同于服务端的字节序转换。因为接收的是服务端IP字符串形式
    serv_addr.sin_port = htons(atoi(argv[2])); // 

    // 服务端调用listen后，客户端可请求连接
    // 调用connect 操作系统（内核）自动给客户端套接字分配IP和端口号，
    // 无需调用bind函数进行分配
    // 调用connect 要么 服务端接收连接请求（进入等待队列即可） 或者 断网等异常情况而中断连接请求，否则 connect不会返回
    if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handing("connect() error!");
    }

    // 数据读取
    str_len = read(sock, message, sizeof(message) - 1);
    if (str_len == -1)
    {
        error_handing("read() error!");
    }

    printf("Message from server : %s \n", message);

    // 关闭socket
    close(sock);

    return 0;
}

void error_handing(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
