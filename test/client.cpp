//sgn

#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFSIZE (10*1024)

int main() {
    int sockfd, recvd;
    char buf[BUFFSIZE];
    std::string strMsg = "WRITE sgn test write 01";
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in their_addr;
    struct hostent* he = gethostbyname("192.168.83.129");
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(10001);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    
    sendto(sockfd, strMsg.c_str(), strMsg.length(), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
    std::cout << "Sent msg from port " << their_addr.sin_port << std::endl;



    std::cout << "Waiting for response" << std::endl;
    bzero(buf, BUFFSIZE);
    recvd = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *) &their_addr, (socklen_t*)&their_addr);
    buf[recvd] = '\0';

    std::string strHost = std::string(inet_ntoa(their_addr.sin_addr));
    std::cout << "Received " << buf << " from " << strHost << ":" << their_addr.sin_port << std::endl;
    return 0;
}