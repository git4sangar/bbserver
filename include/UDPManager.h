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

namespace util {
	class UDPListener {
	public:
		typedef std::shared_ptr<UDPListener> Ptr;
		UDPListener() {}
		~UDPListener() {}
		virtual bool onNetPacket(const struct sockaddr *pClientAddr, const std::string& pPkt) = 0;
	};

	class UDPManager : public TimerListener, public std::enable_shared_from_this<UDPManager>
	{
	public:
		typedef std::shared_ptr<UDPManager> Ptr;

		UDPManager(UDPListener::Ptr pListener, unsigned int pPort)
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
		UDPListener::Ptr mpListener;
		unsigned int mPort;
		Logger& mLogger;
		Timer* mpTimer;
		std::map<size_t, struct sockaddr_in *> mGarbage;
	};

} // namespace util