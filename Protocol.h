//sgn
#pragma once

#include <iostream>
#include <memory>

#include "Constants.h"
#include "ReadWriteLock.h"
#include "TCPManager.h"
#include "FileManager.h"
#include "FileLogger.h"
#include "Timer.h"
//	#include "StateMachine.h" // Circular Dependency, so commented

using namespace util;

class StateMachine;
class Protocol : public TCPListener, public TimerListener, public std::enable_shared_from_this<Protocol>
{
public:
	typedef std::shared_ptr<Protocol> Ptr;
	Protocol(FileManager::Ptr pFileMgr, ReadWriteLock::Ptr pRdWrtLock);
	~Protocol() {}

	void init();
	void broadcastMessage(const std::string& strMsg);

	void addToTimer() {
		mActiveTimeoutId = mpTimer->pushToTimerQueue(shared_from_this(), PRECOMMIT_TIMEOUT_SECS);
		mLogger << "Protocol : Added to Timer with id " << mActiveTimeoutId << std::endl;
	}
	
	void removeActiveTimer() {
		mpTimer->removeFromTimerQueue(mActiveTimeoutId);
		mActiveTimeoutId = 0;
	}

	bool onWriteOrReplace(int32_t connfd, const std::string& pSender, const std::string& pMsg, size_t pReplaceNo = 0);
	void onConnect(int32_t connfd) {}
	void onDisconnect(int32_t connfd) {}
	bool onNetPacket(int32_t connfd, std::string pHost, const std::string& pPkt);
	void onTimeout(size_t pTimeoutId);

	void respondBackToPeer(std::string pHost, uint32_t pPort, const std::string& pMsg) {
		mpNetMgrSync->sendPacket(pHost, pPort, pMsg);
	}
	void sendWriteResponse(const std::string& pMsg, size_t pMsgNo = 0);

	void grabReadLock() { mpRdWrtLock->read_lock(); }
	void grabWriteLock() { mpRdWrtLock->write_lock(); }
	void releaseReadLock() { mpRdWrtLock->read_unlock(); }
	void releaseWriteLock() { mpRdWrtLock->write_unlock(); }
	void forceQuit();

	size_t writeOrReplaceMsg() {
		size_t msgNo = mpFileMgr->writeOrReplace(mSender, mMsgToWrite, mReplaceNo);
		return msgNo;
	}

	void undoWrite() {
		mpFileMgr->undoLastWritten();
	}
	void clearAll();

	TCPListener::Ptr getNetListenerPtr() { return shared_from_this(); }
	Protocol::Ptr getSharePtr() { return shared_from_this(); }

private:
	bool parseCommitPacket(const std::string& pPkt);
	uint32_t parsePort(std::string& pPkt);

	TCPManager::Ptr mpNetMgrSync;
	FileManager::Ptr mpFileMgr;
	ReadWriteLock::Ptr mpRdWrtLock;
	ConfigManager* mpCfgMgr;
	Timer* mpTimer;
	StateMachine* mpCurState;

	unsigned int mPositiveAcks, mSuccessCount;
	size_t mActiveTimeoutId, mReplaceNo;
	std::string mMsgToWrite, mSender;
	Logger& mLogger;
	int32_t mpSenderSockAddr;
};
