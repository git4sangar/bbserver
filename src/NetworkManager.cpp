//sgn
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "NetworkManager.h"

#define MODULE_NAME "NetworkManager : "
namespace util {

void NetworkManager::sendPacket(std::string pHost, unsigned int pPort, std::string strMsg) {
    mLogger << MODULE_NAME << "Sending \"" << strMsg << "\" to " << pHost << ":" << pPort << std::endl;
    int sockfd = 0;
    struct hostent *he;

    struct sockaddr_in their_addr;
    memset(&their_addr, 0, sizeof(their_addr));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    he = gethostbyname(pHost.c_str());
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(pPort);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    sendto(sockfd, strMsg.c_str(), strMsg.length(), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
    close(sockfd);
}

void NetworkManager::sendPacket(const struct sockaddr* pClientaddr, const std::string strMsg) {
   const struct sockaddr_in *pClientAddrIn = (const struct sockaddr_in *)pClientaddr;
   mLogger << MODULE_NAME << "Sending packet " << strMsg << " to " << inet_ntoa(pClientAddrIn->sin_addr) << std::endl;
   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   sendto(sockfd, strMsg.c_str(), strMsg.length(), 0, pClientaddr, sizeof(struct sockaddr));
   close(sockfd);
}

void NetworkManager::receiveThread(unsigned int pPort) {
   int sockfd;
   sockfd = socket(AF_INET, SOCK_DGRAM, 0);

   int optval = 1;
   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

   struct sockaddr_in serveraddr, clientaddr;
   memset(&serveraddr, 0, sizeof(serveraddr));
   memset(&clientaddr, 0, sizeof(clientaddr));

   serveraddr.sin_family = AF_INET;
   serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
   serveraddr.sin_port = htons(pPort);

   bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
   mLogger << MODULE_NAME << "Bound socket to port " << pPort << std::endl;

   struct sockaddr_in ;
   int clientlen = sizeof(clientaddr), recvd;
   char buf[BUFFSIZE];

   while(1) {
      bzero(buf, BUFFSIZE);
      recvd = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *) &clientaddr, (socklen_t*)&clientlen);
      buf[recvd] = '\0';
      std::string strHost = std::string(inet_ntoa(clientaddr.sin_addr));
      mLogger << MODULE_NAME << "Received " << buf << " from " << strHost << ":" << clientaddr.sin_port << std::endl;
      mpListener->onNetPacket((struct sockaddr *)&clientaddr, std::string(buf));
   }
}

} // namespace util
