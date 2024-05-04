#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <functional>

int main(int argc, char const *argv[]) {
    if (argc <= 2) {
        std::cout << "usage : " << basename(argv[0]) << " ip_address port_number" << std::endl;
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    assert (ret >= 0);

    const char* oob_data = "oob_data";
    const char* normal_data = "normal_data";
    send(sockfd, normal_data, strlen(normal_data), 0);
    send(sockfd, oob_data, strlen(oob_data), MSG_OOB);  // 只有最后一个字节 'a' 被当作带外数据，其余的 "oob_dat" 当作普通数据
    send(sockfd, normal_data, strlen(normal_data), 0);
    
    close(sockfd);
    return 0;
}
