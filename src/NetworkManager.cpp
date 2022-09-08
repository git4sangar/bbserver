//sgn
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "Constants.h"
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

   struct sockaddr_in serveraddr;
   memset(&serveraddr, 0, sizeof(serveraddr));

   serveraddr.sin_family = AF_INET;
   serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
   serveraddr.sin_port = htons(pPort);

   bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
   mLogger << MODULE_NAME << "Bound socket to port " << pPort << std::endl;

   int clientlen = sizeof(struct sockaddr_in), recvd;
   char buf[BUFFSIZE];

   while(1) {
      struct sockaddr_in *pClientAddr = new struct sockaddr_in;
      memset(pClientAddr, 0, sizeof(struct sockaddr_in));
      memset(buf, 0, BUFFSIZE);

      recvd = recvfrom(sockfd, buf, BUFFSIZE, 0, (struct sockaddr *) pClientAddr, (socklen_t*)&clientlen);
      buf[recvd] = '\0';
      if(std::string(buf) == "QUIT") {
         close(sockfd);
         mLogger << MODULE_NAME << "Quitting Recive Thread bound to port " << pPort << std::endl;
         return;
      }

      std::string strHost = std::string(inet_ntoa(pClientAddr->sin_addr));
      mLogger << MODULE_NAME << "Received \"" << buf << "\" from " << strHost << ":" << pClientAddr->sin_port << std::endl;

      size_t timerId = mpTimer->pushToTimerQueue(getTimerListener(), GARBAGE_TIMEOUT_SEC);
      mGarbage.insert({timerId, pClientAddr});

      pThreadPool->push_task(&NetworkListener::onNetPacket, mpListener, (struct sockaddr *)pClientAddr, std::string(buf));
      //mpListener->onNetPacket((struct sockaddr *)pClientAddr, std::string(buf));
   }
}

} // namespace util
