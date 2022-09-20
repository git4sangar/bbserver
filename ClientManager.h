//sgn
#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include "ConfigManager.h"
#include "ReadWriteLock.h"
#include "TCPManager.h"
#include "Timer.h"
#include "Protocol.h"
#include "FileManager.h"
#include "FileLogger.h"

using namespace util;

class Protocol;
class ClientManager : public TCPListener, public std::enable_shared_from_this<ClientManager>
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
	void init() { mpNetMagr = std::make_shared<TCPManager>(getNetListenerPtr(), mpCfgMgr->getBPort()); }
	virtual ~ClientManager() {}

	TCPListener::Ptr getNetListenerPtr() { return shared_from_this(); }
	void handleClientCmd(std::string strHost, std::string strPkt) {}

	void onConnect(int32_t connfd);
	void onDisconnect(int32_t connfd);
	bool onNetPacket(int32_t connfd, std::string pHost, const std::string& pPkt);
	
	void onSigHup();

private:
	std::string handleUserCmd(int32_t connfd, const std::string pPkt);
	std::string handleReadCmd(const std::string& pPkt);
	std::string handleWriteCmd(int32_t connfd, const std::string& pPkt);
	std::string handleReplaceCmd(int32_t connfd, const std::string& pPkt);
	std::string myTrim(std::string str);
	bool checkLength(const std::string& pPkt, const std::string& pCmd) { return (pPkt.length() > pCmd.length()); }

	std::unordered_map<int32_t, std::string> mHost2UserMap;
	FileManager::Ptr mpFileMgr;
	ReadWriteLock::Ptr mpRdWrLock;
	Protocol::Ptr mpProtocol;
	TCPManager::Ptr mpNetMagr;
	ConfigManager* mpCfgMgr;
	Timer* mpTimer;
	Logger& mLogger;
};