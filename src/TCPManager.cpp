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

void TCPManager::sendPacket(std::string pHost, unsigned int pPort, std::string strMsg) {
    int32_t sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in their_addr;
    struct hostent* he = gethostbyname(pHost.c_str());
    their_addr.sin_family   = AF_INET;
    their_addr.sin_port     = htons(pPort);
    their_addr.sin_addr     = *((struct in_addr *)he->h_addr);

    int32_t ret = connect(sockfd, (struct sockaddr *)&their_addr, sizeof(their_addr));
    if(ret == -1) {
        mLogger << MODULE_NAME << "Could not connect to " << pHost << ":" << pPort << std::endl;
        return;
    }
    mLogger << MODULE_NAME << "Sending packet \"" << strMsg << "\"" << std::endl;
    write(sockfd, strMsg.c_str(), strMsg.length());

    // close the connection gracefully
    pThreadPool->push_task([=]{
        char buff[SMALL_BUFFER];
        int32_t recvd = read(sockfd, buff, SMALL_BUFFER-1);
        buff[recvd] = '\0';
        mLogger << MODULE_NAME << "Closing connection made for sending packet \"" << strMsg << "\"" << std::endl;
        close(sockfd);}
    );
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
        for(const auto [sock, host] : mClientSocks) {
            FD_SET(sock, &mReadDescriptors);
            if(sock > maxDescriptor) maxDescriptor = sock;
        }

        int32_t activity = select(maxDescriptor + 1 , &mReadDescriptors , NULL , NULL , NULL);
        if(activity == -1) {mLogger << MODULE_NAME << "select error " << errno << std::endl; continue; }

        if(FD_ISSET(mMasterSocket, &mReadDescriptors)) {
            struct sockaddr_in cliaddr;
            int32_t addrlen = sizeof(cliaddr);

            int newSock         = accept(mMasterSocket,  (struct sockaddr *)&cliaddr, (socklen_t*)&addrlen);
            std::string strHost = inet_ntoa(cliaddr.sin_addr);
            mLogger << MODULE_NAME << "Accepting a connection from "
                    << strHost << ":" << ntohs(cliaddr.sin_port)
                    << " New file descriptor " << newSock
                    << std::endl;
            mClientSocks.insert({ newSock, strHost });
            pThreadPool->push_task(&TCPListener::onConnect, mpListener, newSock);
        } else {
            for(auto itr = mClientSocks.begin(); itr != mClientSocks.end();) {
                if(FD_ISSET(itr->first, &mReadDescriptors) ) {
                    recd = read(itr->first, buff, BUFFSIZE-1);
                    buff[recd] = '\0';

                    if(recd == 0) {
                        mLogger << MODULE_NAME << "Got disconnect request" << std::endl;
                        mClientSocks.erase(itr);
                        pThreadPool->push_task(&TCPListener::onDisconnect, mpListener, itr->first);
                        close(itr->first);
                    } else {
                        if(std::string(buff) == SELFQUIT) { mLogger << MODULE_NAME << "Quitting" << std::endl; return; }
                        mLogger << MODULE_NAME << "Got a packet " << buff << std::endl;
                        pThreadPool->push_task(&TCPListener::onNetPacket, mpListener, itr->first, itr->second, std::string(buff));
                    }
                    itr = mClientSocks.end();
                }
                else itr++;
            }
        }
    }
}

}   // namespace util