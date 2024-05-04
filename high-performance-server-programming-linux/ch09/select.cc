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
#include <iostream>

/*
    同时接收普通数据和带外数据
    socket 上接收到普通数据和带外数据都使 select 返回，但是 socket 处于不同的就绪状态：
        前者处于可读状态，后者处于异常状态
*/

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
    assert(connfd >= 0);

    char buf[1024];
    fd_set read_fds;        // 可读 fd 集合，fdset 是一个整型数组
    fd_set exception_fds;   // 异常 fd 集合
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while (1) {

        memset(buf, '\0', sizeof(buf));
        // 每次调用 Select 前都需要重新在 read_fds 和 exception_fds 中设置文件描述符 connfd, 
        // 因为 事件发生之后，文件描述符集合将被内核修改
        FD_SET(connfd, &read_fds);      // 设置 read_fds 的 connfd 位
        FD_SET(connfd, &exception_fds); // 设置 exception_fds 的 connfd 位
        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);

        if (ret < 0) {
            std::cout << "selection failture";
            break;
        }

        // 可读事件，使用 普通的 recv() 读取数据
        if (FD_ISSET(connfd, &read_fds)) {  // read_fds 的 connfd 位是否被设置
            ret = recv(connfd, buf, sizeof(buf) - 1, 0);
            if (ret < 0) {
                break;
            }
            std::cout << "get " << ret << " bytes from <normal> data: " << buf << std::endl;    
        } else if (FD_ISSET(connfd, &exception_fds)){  // 异常事件，使用 MSG_OOB 标志的 recv 函数读取带外数据
            ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
            if (ret < 0) {
                break;
            }   
            std::cout << "get " << ret << " bytes from <oob> data: " << buf << std::endl;    
        }
    }
    close(connfd);
    close(listenfd);
    return 0;
}
