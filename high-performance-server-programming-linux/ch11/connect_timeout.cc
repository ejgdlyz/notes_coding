#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>

// 超时连接函数
int timeout_connect(const std::string& ip, int port, int time) {
    int ret = 0;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    // 通过 SO_RCVTIMEO 和 SO_SNDTIMEO 所设置的超时时间类型是 timeval，与 select 系统调用的超时参数类型相同
    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret != -1);

    ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    std::cout << "ret = " << ret << std::endl;
    if (ret == -1) {
        // 超时对应的错误号是 EINPROGRESS
        if (errno == EINPROGRESS) {
            std::cout << "connecting timeout, process timeout logic" << std::endl;
        } else {
            std::cout << "error occur when connnecting to server" << std::endl;
        }
        return -1;
    }
    return sockfd;
}

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }
    const std::string ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = timeout_connect(ip, port, 3);
    if (sockfd < 0) {
        return 1;
    }
    return 0;
}

/*
$ ./connect_timeout 192.168.10.222 9999
ret = -1
connecting timeout, process timeout logic
*/