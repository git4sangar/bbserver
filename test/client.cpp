//sgn

#include <iostream>
#include <strings.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ThreadPool.h"

#define BUFFSIZE (10*1024)

void recvTask(int sockfd, struct sockaddr_in *pToAddr, socklen_t *pLen) {
    char buf[BUFFSIZE];
    int recvd = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *) pToAddr, (socklen_t*)pLen);
    buf[recvd] = '\0';
    std::string strHost = std::string(inet_ntoa(pToAddr->sin_addr));
    std::cout << "Received " << buf << " from " << strHost << ":" << pToAddr->sin_port << std::endl;
}

int main() {
    int sockfd, recvd;
    char buf[BUFFSIZE];
    std::string strMsg = "REPLACE 1000/sgn test write write again 1000";
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in their_addr;
    struct hostent* he = gethostbyname("192.168.83.129");
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(10001);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    
    sendto(sockfd, strMsg.c_str(), strMsg.length(), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
    std::cout << "Sent msg from port " << their_addr.sin_port << std::endl;


    util::ThreadPool pool(2);
    socklen_t len = sizeof(struct sockaddr_in);
    std::future<void> recvFuture = pool.submit(recvTask, sockfd, &their_addr, &len);
    recvFuture.wait();
    /*std::cout << "Waiting for response" << std::endl;
    bzero(buf, BUFFSIZE);
    recvd = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *) &their_addr, (socklen_t*)&their_addr);
    buf[recvd] = '\0';

    std::string strHost = std::string(inet_ntoa(their_addr.sin_addr));
    std::cout << "Received " << buf << " from " << strHost << ":" << their_addr.sin_port << std::endl;*/
    return 0;
}