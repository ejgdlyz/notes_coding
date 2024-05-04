#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int clnt_sock;
    char buf[BUF_SIZE];
    struct sockaddr_in serv_adr;

    FILE* readfp;
    FILE* writefp;

    clnt_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(clnt_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    readfp = fdopen(clnt_sock, "r");
    writefp = fdopen(clnt_sock, "w");

    while(1)
    {
        if (fgets(buf, sizeof(buf), readfp) == NULL)  // 接收到 EOF, 返回 NULL 指针
        {
            break;
        }
        fputs(buf, stdout);
        fflush(stdout);
    }
 
    fputs("FROM CLIENT: Thank you! \n", writefp);  // 向服务端发送最后的字符串
    fflush(writefp);
    
    fclose(writefp);  
    fclose(readfp);

    return 0;
}