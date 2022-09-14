//sgn
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "Constants.h"
#include "ClientManager.h"

#define MODULE_NAME "ClientManager : "

using namespace util;

std::string ClientManager::handleUserCmd(int32_t connfd, const std::string pPkt) {
	mLogger << MODULE_NAME << "handleUserCmd {" << std::endl;
	std::stringstream ss;

	if (pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t ") != std::string::npos) {
		mLogger << MODULE_NAME << "User Cmd invalid packet " << pPkt << std::endl;
		return "1.2 ERROR USER Invalid text";
	}
	if (mHost2UserMap.count(connfd) > 0) {
		mLogger << MODULE_NAME << "User " << mHost2UserMap[connfd] << " existing already" << std::endl;
		ss.str(""); ss << "HELLO " << mHost2UserMap[connfd] << ", how are you?";
		return ss.str();
	}
	if (pPkt.length() > USER.length()) {
		std::string userName;
		userName = pPkt.substr(USER.length());
		userName = myTrim(userName);
		mHost2UserMap.insert({ connfd, userName });
		mLogger << MODULE_NAME << "Adding new user " << userName << std::endl;

		ss.str(""); ss << "1.0 HELLO " << userName << ", how are you?";
		return ss.str();
	}
	return "1.2 ERROR USER Invalid text";
}

std::string ClientManager::handleReadCmd(const std::string& pPkt) {
	mLogger << MODULE_NAME << "handleReadCmd {" << std::endl;
	if (pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t ") != std::string::npos)
		return "ERROR READ Invalid message number";

	std::string strMsgNo, strMsg;

	strMsgNo = pPkt.substr(READ.length());
	strMsgNo = myTrim(strMsgNo);
	size_t ulMsgNo = std::stoul(strMsgNo);

	mpRdWrLock->read_lock();
	strMsg = mpFileMgr->readFromFile(ulMsgNo);
	mpRdWrLock->read_unlock();

	mLogger << MODULE_NAME << "Read msg " << strMsgNo << " : " << strMsg << std::endl;

	if (strMsg.empty())
		return std::string("2.1 UNKNOWN ") + strMsgNo + " message does not exist";

	size_t pos = strMsg.find_first_of("/");
	if (pos == std::string::npos)
		return "2.2 ERROR READ Corrupted Entry";
	strMsg[pos] = ' ';

	return std::string("2.0 MESSAGE ") + strMsg;
}

std::string ClientManager::handleWriteCmd(int32_t connfd, const std::string& pPkt) {
	mLogger << MODULE_NAME << "handleWriteCmd {" << std::endl;

	if(pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t ") != std::string::npos) return "ERROR WRITE invalid input";
	std::string strSender = "nobody";
	if (mHost2UserMap.count(connfd) > 0) strSender = mHost2UserMap[connfd];

	std::string strMsg = pPkt.substr(WRITE.length());
	strMsg = myTrim(strMsg);

	bool isOk = mpProtocol->onWriteOrReplace(connfd, strSender, strMsg);
	mLogger << MODULE_NAME << "Write req triggered : " << isOk << " (0-> failed, 1-> success) and msg " << pPkt << std::endl;
	if (isOk)
		return std::string();

	return "ERROR WRITE another write/replace in progress";
}

std::string ClientManager::handleReplaceCmd(int32_t connfd, const std::string& pPkt) {
	mLogger << MODULE_NAME << "handleReplaceCmd {" << std::endl;

	if(pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t/ ") != std::string::npos) return "ERROR REPLACE invalid input";
	std::string strSender = "nobody";
	if (mHost2UserMap.count(connfd) > 0) strSender = mHost2UserMap[connfd];

	std::string strMsg = pPkt.substr(REPLACE.length());
	strMsg = myTrim(strMsg);
	auto pos = strMsg.find_first_of("/");
	if(pos == std::string::npos) return "ERROR REPLACE invalid input";

	std::string strMsgNo = strMsg.substr(0, pos);
	size_t msgNo = std::stol(strMsgNo);

	strMsg = strMsg.substr(pos+1);

	bool isOk = mpProtocol->onWriteOrReplace(connfd, strSender, strMsg, msgNo);
	mLogger << MODULE_NAME << "Replace req triggered : " << isOk << " (0-> failed, 1-> success) and msg " << pPkt << std::endl;
	if (isOk)
		return std::string();

	return "ERROR REPLACE another write/replace in progress";
}

bool ClientManager::onNetPacket(int32_t connfd, const std::string& pPkt) {
	std::string strResp;

	if (pPkt.find(USER) == 0 && checkLength(pPkt, USER)) strResp = handleUserCmd(connfd, pPkt);
	else if (pPkt.find(READ) == 0 && checkLength(pPkt, READ)) strResp = handleReadCmd(pPkt);
	else if (pPkt.find(WRITE) == 0 && checkLength(pPkt, WRITE)) strResp = handleWriteCmd(connfd, pPkt);
	else if (pPkt.find(REPLACE) == 0 && checkLength(pPkt, REPLACE)) strResp = handleReplaceCmd(connfd, pPkt);
	else if (pPkt.find(QUIT) == 0) strResp = std::string("4.0 BYE ") + mHost2UserMap[connfd];
	else strResp = "UNKNOWN command";

	write(connfd, strResp.c_str(), strResp.length());
	mLogger << MODULE_NAME << "onNetPacket Response : " << strResp << std::endl;
	return true;
}

void ClientManager::onConnect(int32_t connfd) {
	std::string&& strWelcomeMsg = "0.0 Welcome to BBServer 1.0";
	write(connfd, strWelcomeMsg.c_str(), strWelcomeMsg.length());
}

void ClientManager::onDisconnect(int32_t connfd) {
	mLogger << MODULE_NAME << "4.0 BYE " << std::endl;
	mHost2UserMap.erase(connfd);
	close(connfd);
}

std::string ClientManager::myTrim(std::string str) {
	str = std::regex_replace(str, std::regex("^\\s+"), std::string(""));
	str = std::regex_replace(str, std::regex("\\s+$"), std::string(""));
	return str;
}

void ClientManager::onSigHup() {
	mLogger << MODULE_NAME << "on SigHup" << std::endl;
	mpNetMagr->quitReceiveThread();
	mpProtocol->forceQuit(); // this acquires write lock, so release it at last
	mpFileMgr->close();
	mpFileMgr = nullptr;
	mHost2UserMap.clear();
	mLogger.closeLogger();
	mpRdWrLock->write_unlock();
}