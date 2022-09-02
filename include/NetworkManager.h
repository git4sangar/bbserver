//sgn
#pragma once

#include <iostream>
#include <memory>
#include "FileLogger.h"

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

namespace util {
	class NetworkListener {
	public:
		typedef std::shared_ptr<NetworkListener> Ptr;
		NetworkListener() {}
		~NetworkListener() {}
		virtual void onNetPacket(const std::string& pHost, unsigned int pPort, const std::string& pPkt) = 0;
	};

	class NetworkManager
	{
	public:
		typedef std::shared_ptr<NetworkManager> Ptr;

		NetworkManager(NetworkListener::Ptr pListener, unsigned int pPort)
			: mpListener{ pListener }
			, mPort {pPort}
			, mLogger{ Logger::getInstance() }
		{}
		~NetworkManager() {}

		void sendPacket(std::string pHost, unsigned int pPort, std::string strMsg);
		void receiveThread(unsigned int pPort);

	private:
		NetworkListener::Ptr mpListener;
		unsigned int mPort;
		Logger& mLogger;
	};

} // namespace util