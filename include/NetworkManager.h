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

#define BUFFSIZE 			(10*1024)
#define GARBAGE_TIMEOUT_SEC (60)

namespace util {
	class NetworkListener {
	public:
		typedef std::shared_ptr<NetworkListener> Ptr;
		NetworkListener() {}
		~NetworkListener() {}
		virtual bool onNetPacket(const struct sockaddr *pClientAddr, const std::string& pPkt) = 0;
	};

	class NetworkManager : public TimerListener, public std::enable_shared_from_this<NetworkManager>
	{
	public:
		typedef std::shared_ptr<NetworkManager> Ptr;

		NetworkManager(NetworkListener::Ptr pListener, unsigned int pPort)
			: mpListener{ pListener }
			, mPort {pPort}
			, mLogger{ Logger::getInstance() }
			, mpTimer{ Timer::getInstance() }
		{
			//std::thread tRecv(&NetworkManager::receiveThread, this, pPort); tRecv.detach();
			pThreadPool->push_task(&NetworkManager::receiveThread, this, pPort);
		}
		~NetworkManager() {}

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