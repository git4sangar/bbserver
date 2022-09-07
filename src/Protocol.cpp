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

#define MODULE_NAME "Protocol : "

Protocol::Protocol(FileManager::Ptr pFileMgr, ReadWriteLock::Ptr pRdWrtLock)
	: mpFileMgr{ pFileMgr }
	, mpRdWrtLock {	pRdWrtLock}
	, mpCurState { nullptr }
	, mActiveTimeoutId{0}
	, mLastMsgNo{0}
	, mPositiveAcks{0}
	, mSuccessCount{0}
	, mLogger{ Logger::getInstance() }
	, mpSenderSockAddr{nullptr}
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

	mLogger << MODULE_NAME << "Broadcasting " << strMsg << " to all peers" << std::endl;
	for (const auto& [host, port] : mpCfgMgr->getPeers()) {
		mpNetMgrSync->sendPacket(host, port, strMsg);
	}
}

bool Protocol::onNetPacket(const struct sockaddr *pClientAddr, const std::string& pPkt) {
	StateMachine* pNewState = nullptr;

	mLogger << MODULE_NAME << "Got packet " << pPkt << std::endl;
	//	For Server
	if (pPkt.find(POSITIVE_ACK) == 0 && ++mPositiveAcks == mpCfgMgr->getPeers().size())
		pNewState = mpCurState->onAllAck();
	else if (pPkt.find(SUCCESS) == 0 && ++mSuccessCount == mpCfgMgr->getPeers().size())
		pNewState = mpCurState->onSuccess();
	else if (pPkt.find(UNSUCCESS) == 0)
		pNewState = mpCurState->onUnsuccess();

	//	For clients
	if (pPkt.find(PRECOMMIT) == 0)
		pNewState = mpCurState->onPrecommit(pClientAddr);
	else if (pPkt.find(ABORT) == 0)
		pNewState = mpCurState->onAbort(pClientAddr);
	else if (pPkt.find(SUCCESSFUL) == 0)
		pNewState = mpCurState->onSuccessful(pClientAddr);
	else if (pPkt.find(COMMIT) == 0) {
		parsePacket(pPkt);
		pNewState = mpCurState->onCommit(pClientAddr);
	}
	else if (pPkt.find(UNSUCCESSFUL) == 0)
		pNewState = mpCurState->onUnsuccessful(pClientAddr);

	if (pNewState) {
		if (pNewState->getStateEnum() == States::Idle) clearAll();
		mLogger << MODULE_NAME << "--- " << mpCurState->getStateName() << " --> to --> " << pNewState->getStateName() << " ---" << std::endl;
		mpCurState = pNewState;
	} else { mLogger << MODULE_NAME << pPkt << " is not relevant for cur state : " << mpCurState->getStateName() << std::endl; }

	return true;
}

void Protocol::sendMessageToPeer(const struct sockaddr *pClientAddr, const std::string& pMsg) {
	const struct sockaddr_in *pClientAddrIn = (const struct sockaddr_in *)pClientAddr;
	std::string strHost(inet_ntoa(pClientAddrIn->sin_addr));

	const std::map<std::string, unsigned int>& peers = mpCfgMgr->getPeers();
	if(peers.count(strHost) > 0) {
		mpNetMgrSync->sendPacket(strHost, peers.at(strHost), pMsg);
	}
}

bool Protocol::onWriteRequest(const struct sockaddr *pClientAddr, const std::string& pSender, const std::string& pMsg) {
	mLogger << MODULE_NAME << "OnWrite Request" << std::endl;
	if (mpCurState->getStateEnum() != States::Idle)
		return false;

	StateMachine* pNewState = mpCurState->onWriteRequest();
	if (pNewState) {
		mpSenderSockAddr = pClientAddr;
		mSender = pSender;
		mMsgToWrite = pMsg;
		mLogger << MODULE_NAME << "--- " << mpCurState->getStateName() << " --> to --> " << pNewState->getStateName() << " ---" << std::endl;
		mpCurState = pNewState; 
		return true;
	}
	return false;
}

void Protocol::sendWriteResponse(const std::string& pMsg) {
	std::string strMsg = pMsg;
	if (pMsg.find("WROTE") == 0)
		strMsg = pMsg + std::string(" ") + std::to_string(mLastMsgNo);
	mLogger << MODULE_NAME << "Sending write response " << std::endl;

	mpNetMgrSync->sendPacket(mpSenderSockAddr, pMsg);
	mpSenderSockAddr = nullptr;
}

void Protocol::onTimeout(size_t pTimeoutId) {
	mLogger << MODULE_NAME << "Got timeout for Timer Id " << pTimeoutId << std::endl;
	StateMachine* pNewState = nullptr;
	if (pTimeoutId != mActiveTimeoutId) return;

	mPositiveAcks = 0;
	mSuccessCount = 0;
	pNewState = mpCurState->onNegativeAckOrTimeout();
	if (pNewState) {
		mLogger << MODULE_NAME << "--- " << mpCurState->getStateName() << " --> to --> " << pNewState->getStateName() << " ---" << std::endl;
		mpCurState = pNewState;
	}
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

void Protocol::forceQuit() {
	mpNetMgrSync->quitReceiveThread();
	mpTimer->elapseAllTimersAndQuit();
	mpTimer->waitForTimerQuit();
	grabWriteLock();	// no more read or write
}
