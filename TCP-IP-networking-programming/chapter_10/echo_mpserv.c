#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/*
    多进程回声服务端
*/

#define BUF_SIZE 30
void error_handling(char *message);
void read_childproc(int sig);


int main(int argc, char const *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    
    socklen_t adr_sz;
    int str_len;
    char buf[BUF_SIZE];

    pid_t pid;
    struct sigaction act;
    int state;
    
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    
    // 防止僵尸进程
    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    state = sigaction(SIGCHLD, &act, 0);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, sizeof(serv_adr), 0);
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("build() error");
    }

    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error");
    }
    
    while(1)
    {
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
        if (clnt_sock == -1)
        {
            continue;
        }
        else
        {
            puts("new client connected...");
        }

        pid = fork();  // 复制文件描述符，父子进程分别带有 1 个 生成的套接字文件描述符 clnt_sock 
        if (pid == -1)
        {
            close(clnt_sock);
            continue;
        }
        if (pid == 0) // 子进程的运行区域
        {
            close(serv_sock);  // 关闭服务器套接字文件描述符
            while((str_len = read(clnt_sock, buf, BUF_SIZE)) != 0)
            {
                write(clnt_sock, buf, str_len);
            }
            close(clnt_sock);
            puts("Client disconnected...");
            return 0;  // 子进程终止 产生SIGCHILD信号
        }
        else
        {
            close(clnt_sock);  // 已经通过fork将accept函数创建的套接字描述符复制给子进程，因此服务器端需要销毁自己拥有的文件描述符
        }
    }
    close(serv_sock);

    return 0;
}

void read_childproc(int sig)
{
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG);
    printf("removed proc id: %d \n", pid);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}