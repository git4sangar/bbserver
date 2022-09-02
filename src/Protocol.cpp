//sgn
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <string>
#include <thread>

#include "StateMachine.h"
#include "Constants.h"
#include "Protocol.h"

Protocol::Protocol(FileManager::Ptr pFileMgr, ReadWriteLock::Ptr pRdWrtLock)
	: mpFileMgr{ pFileMgr }
	, mpRdWrtLock {	pRdWrtLock}
	, mpCurState { nullptr }
	, mActiveTimeoutId{0}
	, mLastMsgNo{0}
	, mPositiveAcks{0}
	, mSuccessCount{0}
	, mPortToRespondWrite{0}
	, mLogger{ Logger::getInstance() }
{
	mpCfgMgr = ConfigManager::getInstance();
	mpTimer = Timer::getInstance();
}

void Protocol::init() {
	mpNetMgrSync = std::make_shared<NetworkManager>(getNetListenerPtr(), mpCfgMgr->getSyncPort());
	mpCurState = StateMachine::getInstance(States::Idle, getSharePtr());
}

void Protocol::broadcastMessage(const std::string& pMsg) {
	std::string strMsg = pMsg;
	if (strMsg == COMMIT) {
		std::stringstream ss;
		ss << strMsg << "/" << mSender << "/" << mMsgToWrite;
		strMsg = ss.str();
	}
	for (const auto& [host, port] : mpCfgMgr->getPeers()) {
		mpNetMgrSync->sendPacket(host, port, strMsg);
	}
}

void Protocol::onNetPacket(const std::string& pHost, unsigned int pPort, const std::string& pPkt) {
	StateMachine* pNewState = nullptr;

	//	For Server
	if (pPkt.find(POSITIVE_ACK) == 0 && ++mPositiveAcks == mpCfgMgr->getPeers().size())
		pNewState = mpCurState->onAllAck();
	else if (pPkt.find(SUCCESS) == 0 && ++mSuccessCount == mpCfgMgr->getPeers().size())
		pNewState = mpCurState->onSuccess();
	else if (pPkt.find(UNSUCCESS) == 0)
		pNewState = mpCurState->onUnsuccess();

	//	For clients
	if (pPkt.find(PRECOMMIT) == 0)
		pNewState = mpCurState->onPrecommit(pHost, pPort);
	else if (pPkt.find(ABORT) == 0)
		pNewState = mpCurState->onAbort(pHost, pPort);
	else if (pPkt.find(SUCCESSFUL) == 0)
		pNewState = mpCurState->onSuccessful(pHost, pPort);
	else if (pPkt.find(COMMIT) == 0) {
		parsePacket(pPkt);
		pNewState = mpCurState->onCommit(pHost, pPort);
	}
	else if (pPkt.find(UNSUCCESSFUL) == 0)
		pNewState = mpCurState->onUnsuccessful(pHost, pPort);

	if (pNewState) {
		if (pNewState->getStateEnum() == States::Idle) clearAll();
		mpCurState = pNewState;
	}
}

bool Protocol::onWriteRequest(const std::string& pHost, unsigned int pPort, const std::string& pSender, const std::string& pMsg) {
	if (mpCurState->getStateEnum() != States::Idle)
		return false;

	StateMachine* pNewState = mpCurState->onWriteRequest();
	if (pNewState) {
		mHostToRespondWrite = pHost;
		mPortToRespondWrite = pPort;
		mSender = pSender;
		mMsgToWrite = pMsg;
		mpCurState = pNewState; 
		return true;
	}
	return false;
}

void Protocol::sendWriteResponse(const std::string& pMsg) {
	std::string strMsg = pMsg;
	if (pMsg.find("WROTE") == 0)
		strMsg = pMsg + std::string(" ") + std::to_string(mLastMsgNo);
	sendMessage(mHostToRespondWrite, mPortToRespondWrite, strMsg);
	mHostToRespondWrite.clear();
	mPortToRespondWrite = 0;
}

void Protocol::onTimeout(size_t pTimeoutId) {
	StateMachine* pNewState = nullptr;
	if (pTimeoutId != mActiveTimeoutId) return;

	mPositiveAcks = 0;
	mSuccessCount = 0;
	pNewState = mpCurState->onNegativeAckOrTimeout();
	if (pNewState) mpCurState = pNewState;
}

void Protocol::clearAll() {
	mPositiveAcks = mSuccessCount = 0;
	mActiveTimeoutId = 0;
	mMsgToWrite.clear();
	mSender.clear();
}

bool Protocol::parsePacket(const std::string& pPkt) {
	std::vector<std::string> words;
	std::string word;

	std::stringstream ss(pPkt);
	while (std::getline(ss, word, '/'))
		words.push_back(word);
	if (words.size() >= 3) {
		mSender = words[1];	//	Sender
		mMsgToWrite = words[2];	//	Msg
		return true;
	}
	return false;
}
