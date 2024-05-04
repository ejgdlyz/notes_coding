#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/*
        UDP回声客户端
*/

#define BUF_SIZE 30
void error_handing(char* message);

int main(int argc, char const *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_sz;
    
    struct sockaddr_in serv_adr, from_adr;

    if (argc != 3)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);  // 创建udp套接字
    if (sock == -1)
    {
        error_handing("UDP socket create error");
    }

    memset(&serv_adr, 0 , sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    // if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1);  // 创建已连接套接字
    // {
    //     error_handing("connect error");
    // }
    
    while (1)
    {
        fputs("Insert message(q to quit): ", stdout);
        fgets(message, sizeof(message), stdin);
        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
        {
            break;
        }
        
        // 用于传输数据的UDP套接字描述符, 保存待传输数据的缓冲区地址，待传输数据的长度
        // 可选参数，存有目标地址信息的socketaddr结构体变量的地址，socketaddr 地址长度
        // 首次调用sendto函数自动给套接字分配ip和端口号，且分配的地址一直保留到程序结束为止
        sendto(sock, message, strlen(message), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        // sendto 函数需要经历三个过程 （未连接UDP套接字）
        /*
        1 向UDP套接字注册 目标IP和端口号
        2 传输数据
        3 删除UDP套接字中注册的 目标地址信息
        */
       // 而 已连接套接字则可以省去耗时的1，3 步骤，提高整体通信性能

        adr_sz = sizeof(from_adr);

        // 用于接收数据的udp套接字描述符, 缓冲区地址，可接收的最大字节数，
        // 可选参数一般为0，存发送端地址信息的sockaddr结构体变量地址，发送端sockaddr长度
        str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr*)&from_adr, &adr_sz);
        
        message[str_len] = 0;
        printf("Message from server: %s", message);
    }
    
    close(sock);

    return 0;
}

void error_handing(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
