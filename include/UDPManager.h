//sgn
#pragma once

#include <iostream>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <map>

#include "Constants.h"
#include "Timer.h"
#include "ThreadPool.h"
#include "NetworkListener.h"

#define BUFFSIZE 			(10*1024)
#define GARBAGE_TIMEOUT_SEC (60)

namespace util {
	class UDPManager : public TimerListener, public std::enable_shared_from_this<UDPManager>
	{
	public:
		typedef std::shared_ptr<UDPManager> Ptr;

		UDPManager(NetworkListener::Ptr pListener, unsigned int pPort)
			: mpListener{ pListener }
			, mPort {pPort}
			, mLogger{ Logger::getInstance() }
			, mpTimer{ Timer::getInstance() }
		{
			//std::thread tRecv(&UDPManager::receiveThread, this, pPort); tRecv.detach();
			pThreadPool->push_task(&UDPManager::receiveThread, this, pPort);
		}
		~UDPManager() {}

		void sendPacket(std::string pHost, unsigned int pPort, std::string strMsg);
		void sendPacket(const struct sockaddr* pClientaddr, std::string strMsg);
		void receiveThread(unsigned int pPort);
		void quitReceiveThread() { sendPacket("localhost", mPort, "QUIT"); }

		void onTimeout(size_t pTimeoutId) { delete mGarbage[pTimeoutId]; mGarbage.erase(pTimeoutId); } 	
		TimerListener::Ptr getTimerListener() { return shared_from_this(); }

	private:
		NetworkListener::Ptr mpListener;
		unsigned int mPort;
		Logger& mLogger;
		Timer* mpTimer;
		std::map<size_t, struct sockaddr_in *> mGarbage;
	};

} // namespace util