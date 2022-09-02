//sgn
#pragma once

#include <iostream>
#include <memory>

#include "Constants.h"
#include "ReadWriteLock.h"
#include "NetworkManager.h"
#include "FileManager.h"
#include "FileLogger.h"
#include "Timer.h"
//	#include "StateMachine.h" // Circular Dependency, so commented

using namespace util;

class StateMachine;
class Protocol : public NetworkListener, public TimerListener, public std::enable_shared_from_this<Protocol>
{
public:
	typedef std::shared_ptr<Protocol> Ptr;
	Protocol(FileManager::Ptr pFileMgr, ReadWriteLock::Ptr pRdWrtLock);
	~Protocol() {}

	void init();
	void broadcastMessage(const std::string& strMsg);

	void addToTimer() {
		mActiveTimeoutId = mpTimer->pushToTimerQueue(shared_from_this(), PRECOMMIT_TIMEOUT_SECS);
	}
	void removeActiveTimer() {
		mpTimer->removeFromTimerQueue(mActiveTimeoutId);
		mActiveTimeoutId = 0;
	}

	bool onWriteRequest(const std::string& pHost, unsigned int pPort, const std::string& pSender, const std::string& pMsg);
	void onNetPacket(const std::string& pHost, unsigned int pPort, const std::string& pPkt);
	void onTimeout(size_t pTimeoutId);

	void sendMessage(std::string pHost, unsigned int pPort, const std::string& pMsg) {
		mpNetMgrSync->sendPacket(pHost, pPort, pMsg);
	}
	void sendWriteResponse(const std::string& pMsg);

	void grabReadLock() { mpRdWrtLock->read_lock(); }
	void grabWriteLock() { mpRdWrtLock->write_lock(); }
	void releaseReadLock() { mpRdWrtLock->read_unlock(); }
	void releaseWriteLock() { mpRdWrtLock->write_unlock(); }

	bool writeMsg() {
		mLastMsgNo = mpFileMgr->writeToFile(mSender, mMsgToWrite);
		return (mLastMsgNo > 0);
	}
	void undoWrite() {
		mpFileMgr->undoLastWritten();
	}
	void clearAll();

	NetworkListener::Ptr getNetListenerPtr() { return shared_from_this(); }
	Protocol::Ptr getSharePtr() { return shared_from_this(); }

private:
	bool parsePacket(const std::string& pPkt);

	NetworkManager::Ptr mpNetMgrSync;
	FileManager::Ptr mpFileMgr;
	ReadWriteLock::Ptr mpRdWrtLock;
	ConfigManager* mpCfgMgr;
	Timer* mpTimer;
	StateMachine* mpCurState;

	unsigned int mPositiveAcks, mSuccessCount, mPortToRespondWrite;
	size_t mActiveTimeoutId, mLastMsgNo;
	std::string mMsgToWrite, mSender, mHostToRespondWrite;
	Logger& mLogger;
};
