//sgn
#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <map>

#include "Constants.h"
#include "Timer.h"
#include "ThreadPool.h"

namespace util {
    class TCPListener {
	public:
		typedef std::shared_ptr<TCPListener> Ptr;
		TCPListener() {}
		~TCPListener() {}
        virtual void onConnect(int32_t connfd) = 0;
        virtual void onDisconnect(int32_t connfd) = 0;
		virtual bool onNetPacket(int32_t connfd, const std::string& pPkt) = 0;
	};

    class TCPManager {
    public:
		typedef std::shared_ptr<TCPManager> Ptr;
        TCPManager(TCPListener::Ptr pListener, unsigned int pPort)
			: mpListener{ pListener }
			, mPort {pPort}
			, mLogger{ Logger::getInstance() }
            , mMasterSocket(0)
		{
			//std::thread tRecv(&TCPManager::receiveThread, this, pPort); tRecv.detach();
			pThreadPool->push_task(&TCPManager::connectionManager, this, pPort);
		}
		~TCPManager() {}

		void connectionManager(unsigned int pPort);
		void quitReceiveThread();

    private:
		TCPListener::Ptr mpListener;
		unsigned int mPort;
        std::vector<int32_t> mClientSocks;
		Logger& mLogger;
        int32_t mMasterSocket;
        fd_set mReadDescriptors;
    };
}   // namespace util