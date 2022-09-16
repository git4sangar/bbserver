//sgn
#include <iostream>

#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFSIZE (10*1024)

int main(int argc, char *argv[]) {
    if(argc != 3) {
        std::cout << "Usage: ./TCPClient IP-address Port-no" << std::endl
                << "Eg ./TCPClient localhost 3200" << std::endl
                << "Eg ./TCPClient 192.168.83.129 3200" << std::endl;
        return 0;
    }

    int sockfd, recvd;
    char buf[BUFFSIZE];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in their_addr;
    struct hostent* he = gethostbyname(argv[1]);
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(std::stoi(argv[2]));
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    int32_t ret = connect(sockfd, (struct sockaddr *)&their_addr, sizeof(their_addr));
    if(ret == -1) {
        std::cout << "Error connecting to server errno : " << errno << std::endl;
        return 0;
    }

    while(true) {
        recvd = read(sockfd, buf, BUFFSIZE);
        buf[recvd] = '\0';
        std::cout << buf << std::endl;
        if(std::string(buf).find("BYE") != std::string::npos) break;

        recvd = 0;
        while(recvd == 0) {
            while ((buf[recvd] = getchar()) != '\n') recvd++;
            buf[recvd] = '\0';
        }

        write(sockfd, buf, recvd);
    }

    close(sockfd);
    return 0;
}
