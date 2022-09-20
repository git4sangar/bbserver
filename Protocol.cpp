//sgn
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

#include "StateMachine.h"
#include "Constants.h"
#include "Protocol.h"

#define MODULE_NAME "Protocol : "

Protocol::Protocol(FileManager::Ptr pFileMgr, ReadWriteLock::Ptr pRdWrtLock)
	: mpFileMgr{ pFileMgr }
	, mpRdWrtLock {	pRdWrtLock}
	, mpCurState { nullptr }
	, mActiveTimeoutId{0}
	, mReplaceNo{0}
	, mPositiveAcks{0}
	, mSuccessCount{0}
	, mLogger{ Logger::getInstance() }
	, mpSenderSockAddr{0}
{
	mpCfgMgr = ConfigManager::getInstance();
	mpTimer = Timer::getInstance();
}

void Protocol::init() {
	mpNetMgrSync = std::make_shared<TCPManager>(getNetListenerPtr(), mpCfgMgr->getSyncPort());
	mpCurState = StateMachine::getInstance(States::Idle, getSharePtr());
}

void Protocol::broadcastMessage(const std::string& pCmd) {
	std::string strCmd = pCmd;
	if (strCmd == COMMIT) {
		std::stringstream ss;
		ss << strCmd << "/" << mSender << "/" << mMsgToWrite << "/" << mReplaceNo;
		strCmd = ss.str();
	}

	mLogger << MODULE_NAME << "Broadcasting " << strCmd << " to all peers" << std::endl;
	strCmd += '$';
	strCmd += std::to_string(mpCfgMgr->getSyncPort());
	for (const auto& [host, port] : mpCfgMgr->getPeers()) {
		mpNetMgrSync->sendPacket(host, port, strCmd);
	}
}

bool Protocol::onNetPacket(int32_t connfd, std::string pHost, const std::string& pPkt) {
	StateMachine* pNewState = nullptr;
	std::string strPkt(pPkt);

	if(connfd > 0) write(connfd, QUIT.c_str(), QUIT.length());	// Tear it down, we no more need it
	uint32_t port = parsePort(strPkt);

	mLogger << MODULE_NAME << "Got packet " << strPkt << std::endl;
	//	For Server
	if (strPkt.find(POSITIVE_ACK) == 0 && ++mPositiveAcks == mpCfgMgr->getPeers().size())
		pNewState = mpCurState->onAllAck();
	else if (strPkt.find(SUCCESS) == 0 && ++mSuccessCount == mpCfgMgr->getPeers().size())
		pNewState = mpCurState->onSuccess();
	else if (strPkt.find(UNSUCCESS) == 0)
		pNewState = mpCurState->onUnsuccess();

	//	For clients
	if (strPkt.find(PRECOMMIT) == 0)
		pNewState = mpCurState->onPrecommit(pHost, port);
	else if (strPkt.find(ABORT) == 0)
		pNewState = mpCurState->onAbort(pHost, port);
	else if (strPkt.find(SUCCESSFUL) == 0)
		pNewState = mpCurState->onSuccessful(pHost, port);
	else if (strPkt.find(COMMIT) == 0) {
		parseCommitPacket(strPkt);
		pNewState = mpCurState->onCommit(pHost, port);
	}
	else if (strPkt.find(UNSUCCESSFUL) == 0)
		pNewState = mpCurState->onUnsuccessful(pHost, port);

	if (pNewState) {
		if (pNewState->getStateEnum() == States::Idle) clearAll();
		mLogger << MODULE_NAME << "--- " << mpCurState->getStateName() << " --> to --> " << pNewState->getStateName() << " ---" << std::endl;
		mpCurState = pNewState;
	} else { mLogger << MODULE_NAME << strPkt << " is not relevant for cur state : " << mpCurState->getStateName() << std::endl; }

	return true;
}

bool Protocol::onWriteOrReplace(int32_t connfd, const std::string& pSender, const std::string& pMsg, size_t pReplaceNo) {
	mLogger << MODULE_NAME << "OnWrite Request" << std::endl;
	if (mpCurState->getStateEnum() != States::Idle)
		return false;

	StateMachine* pNewState = mpCurState->onWriteRequest();
	if (pNewState) {
		mpSenderSockAddr = connfd;
		mSender = pSender;
		mMsgToWrite = pMsg;
		mReplaceNo = pReplaceNo;
		mLogger << MODULE_NAME << "--- " << mpCurState->getStateName() << " --> to --> " << pNewState->getStateName() << " ---" << std::endl;
		mpCurState = pNewState; 
		return true;
	}
	return false;
}

void Protocol::sendWriteResponse(const std::string& pMsg, size_t pMsgNo) {
	std::string strMsg = pMsg;
	if(strMsg.find("3.1 UNKNOWN") != std::string::npos) {
		strMsg += std::to_string(mReplaceNo);
	}

	mLogger << MODULE_NAME << "Sending write response " << strMsg << std::endl;

	write(mpSenderSockAddr, strMsg.c_str(), strMsg.length());
	mpSenderSockAddr = 0;
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
	mReplaceNo = mPositiveAcks = mSuccessCount = 0;
	mActiveTimeoutId = 0;
	mMsgToWrite.clear();
	mSender.clear();
}

bool Protocol::parseCommitPacket(const std::string& pPkt) {
	std::vector<std::string> words;
	std::string word;

	std::stringstream ss(pPkt);
	while (std::getline(ss, word, '/'))
		words.push_back(word);
	if (words.size() >= 4) {
		mSender = words[1];	//	Sender
		mMsgToWrite = words[2];	//	Msg
		mReplaceNo = std::stol(words[3]); // Replace No
		return true;
	}
	return false;
}

uint32_t Protocol::parsePort(std::string& pPkt) {
	uint32_t port = 0;
	size_t pos = 0;
	if((pos = pPkt.find_first_of("$")) != std::string::npos) {
		std::string strPort = pPkt.substr(pos+1);
		port = std::stoi(strPort);
		pPkt = pPkt.substr(0, pos);
	}
	return port;
}

void Protocol::forceQuit() {
	mpNetMgrSync->quitReceiveThread();
	mpTimer->elapseAllTimersAndQuit();
	mpTimer->waitForTimerQuit();
	grabWriteLock();	// no more read or write
}
