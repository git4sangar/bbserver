//sgn
#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include "ConfigManager.h"
#include "ReadWriteLock.h"
#include "NetworkManager.h"
#include "Timer.h"
#include "Protocol.h"
#include "FileManager.h"
#include "FileLogger.h"

using namespace util;
class Protocol;
class ClientManager : public NetworkListener, public TimerListener, public std::enable_shared_from_this<ClientManager>
{
public:
	typedef std::shared_ptr<ClientManager> Ptr;
	ClientManager(FileManager::Ptr pFileMgr, ReadWriteLock::Ptr pRdWrLock)
		: mpFileMgr{ pFileMgr }
		, mpRdWrLock{ pRdWrLock }
		, mpCfgMgr{ ConfigManager::getInstance() }
		, mpTimer{ Timer::getInstance() }
		, mLogger { Logger::getInstance() }
	{
		mpProtocol = std::make_shared<Protocol>(pFileMgr, pRdWrLock);
		mpProtocol->init();
	}
	void init() { mpNetMagr = std::make_shared<NetworkManager>(getNetListenerPtr(), mpCfgMgr->getBPort()); }
	virtual ~ClientManager() {}

	NetworkListener::Ptr getNetListenerPtr() { return shared_from_this(); }
	void handleClientCmd(std::string strHost, std::string strPkt) {}

	void onNetPacket(const struct sockaddr *pClientAddr, const std::string& pPkt);
	void onTimeout(size_t pTimeoutId);

private:
	std::string handleUserCmd(const std::string& pHost, const std::string pPkt);
	std::string handleReadCmd(const std::string& pPkt);
	std::string handleWriteCmd(const std::string& pHost, const struct sockaddr *pClientAddr, const std::string& pPkt);
	std::string myTrim(std::string str);

	std::unordered_map<std::string, std::string> mHost2UserMap;
	FileManager::Ptr mpFileMgr;
	ReadWriteLock::Ptr mpRdWrLock;
	Protocol::Ptr mpProtocol;
	NetworkManager::Ptr mpNetMagr;
	ConfigManager* mpCfgMgr;
	Timer* mpTimer;
	Logger& mLogger;
};