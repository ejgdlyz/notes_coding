#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handing(char* message);

int main(int argc, char const *argv[])
{
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr, clnt_adr;
    
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);  // 创建udp套接字
    if (serv_sock == -1)
    {
        error_handing("UDP socket create error");
    }

    memset(&serv_adr, 0 , sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handing("bind() error");
    }

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        // serv_sock 用于接收数据的udp套接字描述符, 缓冲区地址，可接收的最大字节数，
        // 可选参数一般为0，存发送端地址信息的sockaddr结构体变量地址，发送端sockaddr长度
        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

        // printf("message received...\n");   // 必须调用\n刷新输出缓冲区，才能输出该打印语句

        // 用于传输数据的UDP套接字描述符, 保存待传输数据的缓冲区地址，待传输数据的长度
        // 可选参数，存有目标地址信息的socketaddr结构体变量的地址，socketaddr 地址长度
        sendto(serv_sock, message, str_len, 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
    }
    
    close(serv_sock);

    return 0;
}

void error_handing(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
