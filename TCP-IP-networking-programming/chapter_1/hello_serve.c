#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handing(char* message);

int main(int argc, char const *argv[])
{
    // 服务端/客户端套接字
    int serv_sock;
    int clnt_sock;

    // 结构体 保存ip地址与端口等信息
    /*
    struct sockaddr_in {
        sa_family           sin_family;  // 地址族
        uint16_t            sin_port;    // 16位（2字节 short类型）TCP/UDP端口号
        struct in_addr      sin_addr;    // 32位 (4字节) IP地址
        char                sin_zero[8];  // 不使用 8字节
    }
    struct in_addr
    {
        in_addr_t           s_addr;      // 32位 ipv4 地址
    }
    
    struct sockaddr
    {
        sa_family_t         sin_family;   // 地址族(address family)
        char                sa_data[14];  // 地址信息 与上述的 sockaddr_in 14字节对应
    }
    */
    struct sockaddr_in serv_addr;  // p41
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    char message[] = "Hello World!";

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // PF_INET: ipv4 互联网协议族 PF_INET6: ipv4 互联网协议族
    // SOCK_STREAM: 套接字类型 面向连接的套接字 
    // IPPROTO: 具体的协议信息 满足PF_INET, SOCK_STREAM 只有 IPPROTO_TCP，故使用0即可
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);  
    if (serv_sock == -1)
    {
        error_handing("socket() error!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));  // 将 serv_addr 全部置0，以在下文的类型转换 (struct *sockaddr) 时对齐
    serv_addr.sin_family = AF_INET;            // ipv4 地址族
    // 基于字符串的IP地址初始化 serv_addr.sin_addr.s_addr = inet_addr(serv_ip);  p49  
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 把short主机字节序 -> 网络字节序 ,l long类型 4字节  p44
    serv_addr.sin_port = htons(atoi(argv[1]));  // 端口号转为int后再从主机字节序转为网络字节序 , s short 类型 2字节 

    // p51 将初始化的地址信息分配给套接字
    // 将第二个参数指定的地址信息分配给第一个参数中的相应的套接字
    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handing("bind() error!");
    }
    
    // p65 调用listen 进入等待连接请求状态。只有调用listen后，客户端才能调用connect进行连接
    // 调用listen函数后创建请求等待队列，之后客户端即可请求连接
    // 服务端其实作为接收请求的门卫或者一扇门，第二个参数决定了等候队列的长度
    if (listen(serv_sock, 5) == -1)
    {
        error_handing("listen() error!");
    }
    
    // p66 accept 受理客户端连接请求
    // accpet 受理连接请求队列中待处理的客户端连接请求
    // accept 内部将产生用于数据I/O的套接字，并返回文件描述符
    // 该套接字自动创建，并自动与发起来连接请求的客户端建立连接
    // 服务端单独创建的套接字与客户端建立连接后进行数据交换
    // 调用accept时，若等待队列为空，则进入阻塞直到出现新的客户端请求
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1)
    {
        error_handing("accept() error!");
    }

    // 进行数据交换
    write(clnt_sock, message, sizeof(message));

    // 关闭套接字
    close(clnt_sock);
    close(serv_sock);

    return 0;
}

void error_handing(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
