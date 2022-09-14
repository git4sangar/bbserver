//sgn
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "ConfigManager.h"
#include "Constants.h"
#include "TCPManager.h"

#define MODULE_NAME "TCPManager : "

namespace util {

void TCPManager::quitReceiveThread() {
    int32_t sockfd  = socket(AF_INET, SOCK_STREAM, 0);
    uint32_t port   = ConfigManager::getInstance()->getBPort();

    struct sockaddr_in their_addr;
    struct hostent* he = gethostbyname("localhost");
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    connect(sockfd, (struct sockaddr *)&their_addr, sizeof(their_addr));
    write(sockfd, SELFQUIT.c_str(), SELFQUIT.length());
}

void TCPManager::connectionManager(unsigned int pPort) {
    char buff[BUFFSIZE];
    mMasterSocket = socket(AF_INET, SOCK_STREAM, 0);

    int32_t opt = 1;
    setsockopt(mMasterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(pPort);

    mLogger << MODULE_NAME << "Binding to TCP port " << pPort << " and started to listen" << std::endl;
    bind(mMasterSocket, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(mMasterSocket, 5);

    while(true) {
        int32_t maxDescriptor = 0, recd = 0;

        FD_ZERO(&mReadDescriptors);
        FD_SET(mMasterSocket, &mReadDescriptors);
    
        maxDescriptor = mMasterSocket;
        for(int32_t sock : mClientSocks) {
            FD_SET(sock, &mReadDescriptors);
            if(sock > maxDescriptor) maxDescriptor = sock;
        }

        int32_t activity = select(maxDescriptor + 1 , &mReadDescriptors , NULL , NULL , NULL);
        if(activity == -1) {mLogger << MODULE_NAME << "select error " << errno; continue; }

        if(FD_ISSET(mMasterSocket, &mReadDescriptors)) {
            struct sockaddr_in cliaddr;
            int32_t addrlen = sizeof(cliaddr);

            int newSock = accept(mMasterSocket,  (struct sockaddr *)&cliaddr, (socklen_t*)&addrlen);
            mLogger << MODULE_NAME << "Accepting a connection from "
                    << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port)
                    << " New file descriptor " << newSock
                    << std::endl;
            mClientSocks.push_back(newSock);
            pThreadPool->push_task(&TCPListener::onConnect, mpListener, newSock);
        } else {
            for(auto itr = mClientSocks.begin(); itr != mClientSocks.end();) {
                if(FD_ISSET(*itr, &mReadDescriptors) ) {
                    recd = read(*itr , buff, BUFFSIZE-1);
                    buff[recd] = '\0';

                    if(recd == 0) {
                        mLogger << MODULE_NAME << "Got disconnect request" << buff << std::endl;
                        mClientSocks.erase(itr);
                        pThreadPool->push_task(&TCPListener::onDisconnect, mpListener, *itr);
                    } else {
                        if(std::string(buff) == SELFQUIT) { mLogger << MODULE_NAME << "Quitting" << std::endl; return; }
                        mLogger << MODULE_NAME << "Got a packet " << buff << std::endl;
                        pThreadPool->push_task(&TCPListener::onNetPacket, mpListener, *itr, std::string(buff));
                    }
                    itr = mClientSocks.end();
                }
                else itr++;
            }
        }
    }
}

}   // namespace util