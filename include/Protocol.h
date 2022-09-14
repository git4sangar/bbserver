//sgn
#pragma once

#include <iostream>
#include <memory>

#include "Constants.h"
#include "ReadWriteLock.h"
#include "UDPManager.h"
#include "FileManager.h"
#include "FileLogger.h"
#include "Timer.h"
//	#include "StateMachine.h" // Circular Dependency, so commented

using namespace util;

class StateMachine;
class Protocol : public UDPListener, public TimerListener, public std::enable_shared_from_this<Protocol>
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

	bool onWriteOrReplace(const struct sockaddr *pClientAddr, const std::string& pSender, const std::string& pMsg, size_t pReplaceNo = 0);
	bool onNetPacket(const struct sockaddr *pClientAddr, const std::string& pPkt);
	void onTimeout(size_t pTimeoutId);

	void sendMessageToPeer(const struct sockaddr *pClientAddr, const std::string& pMsg);
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

	UDPListener::Ptr getNetListenerPtr() { return shared_from_this(); }
	Protocol::Ptr getSharePtr() { return shared_from_this(); }

private:
	bool parsePacket(const std::string& pPkt);

	UDPManager::Ptr mpNetMgrSync;
	FileManager::Ptr mpFileMgr;
	ReadWriteLock::Ptr mpRdWrtLock;
	ConfigManager* mpCfgMgr;
	Timer* mpTimer;
	StateMachine* mpCurState;

	unsigned int mPositiveAcks, mSuccessCount;
	size_t mActiveTimeoutId, mReplaceNo;
	std::string mMsgToWrite, mSender;
	Logger& mLogger;
	const struct sockaddr *mpSenderSockAddr;
};
