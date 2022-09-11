//sgn
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>

#include "Constants.h"
#include "ClientManager.h"

#define MODULE_NAME "ClientManager : "

std::string ClientManager::handleUserCmd(const std::string& pHost, const std::string pPkt) {
	mLogger << MODULE_NAME << "handleUserCmd {" << std::endl;
	std::stringstream ss;

	if (pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t ") != std::string::npos) {
		mLogger << MODULE_NAME << "User Cmd invalid packet " << pPkt << std::endl;
		return "ERROR USER Invalid text";
	}
	if (mHost2UserMap.count(pHost) > 0) {
		mLogger << MODULE_NAME << "User " << mHost2UserMap[pHost] << " existing already" << std::endl;
		ss.str(""); ss << "HELLO " << mHost2UserMap[pHost] << ", how are you?";
		return ss.str();
	}
	if (pPkt.length() > USER.length()) {
		std::string userName;
		userName = pPkt.substr(USER.length());
		userName = myTrim(userName);
		mHost2UserMap.insert({ pHost, userName });
		mLogger << MODULE_NAME << "Adding new user " << userName << std::endl;

		ss.str(""); ss << "HELLO " << userName << ", how are you?";
		return ss.str();
	}
	return "ERROR USER Invalid text";
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
		return std::string("UNKNOWN ") + strMsgNo + " message does not exist";

	size_t pos = strMsg.find_first_of("/");
	if (pos == std::string::npos)
		return "ERROR READ Corrupted Entry";
	strMsg[pos] = ' ';

	return std::string("MESSAGE ") + strMsg;
}

std::string ClientManager::handleWriteCmd(const std::string& pHost, const struct sockaddr *pClientAddr, const std::string& pPkt) {
	mLogger << MODULE_NAME << "handleWriteCmd {" << std::endl;

	if(pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t ") != std::string::npos) return "ERROR WRITE invalid input";
	std::string strSender = "nobody";
	if (mHost2UserMap.count(pHost) > 0) strSender = mHost2UserMap[pHost];

	std::string strMsg = pPkt.substr(WRITE.length());
	strMsg = myTrim(strMsg);

	bool isOk = mpProtocol->onWriteOrReplace(pClientAddr, strSender, strMsg);
	mLogger << MODULE_NAME << "Write req triggered : " << isOk << " (0-> failed, 1-> success) and msg " << pPkt << std::endl;
	if (isOk)
		return std::string();

	return "ERROR WRITE another write/replace in progress";
}

std::string ClientManager::handleReplaceCmd(const std::string& pHost, const struct sockaddr *pClientAddr, const std::string& pPkt) {
	mLogger << MODULE_NAME << "handleReplaceCmd {" << std::endl;

	if(pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t/ ") != std::string::npos) return "ERROR REPLACE invalid input";
	std::string strSender = "nobody";
	if (mHost2UserMap.count(pHost) > 0) strSender = mHost2UserMap[pHost];

	std::string strMsg = pPkt.substr(REPLACE.length());
	strMsg = myTrim(strMsg);
	auto pos = strMsg.find_first_of("/");
	if(pos == std::string::npos) return "ERROR REPLACE invalid input";

	std::string strMsgNo = strMsg.substr(0, pos);
	size_t msgNo = std::stol(strMsgNo);

	strMsg = strMsg.substr(pos+1);

	bool isOk = mpProtocol->onWriteOrReplace(pClientAddr, strSender, strMsg, msgNo);
	mLogger << MODULE_NAME << "Replace req triggered : " << isOk << " (0-> failed, 1-> success) and msg " << pPkt << std::endl;
	if (isOk)
		return std::string();

	return "ERROR REPLACE another write/replace in progress";
}


bool ClientManager::onNetPacket(const struct sockaddr *pClientAddr, const std::string& pPkt) {
	std::string strResp;
	const struct sockaddr_in* pClient = (const struct sockaddr_in*)pClientAddr;
	std::string pHost = std::string(inet_ntoa(pClient->sin_addr));

	mLogger << MODULE_NAME << "Got packet " << pPkt << std::endl;
	if (pPkt.find(USER) == 0 && checkLength(pPkt, USER)) strResp = handleUserCmd(pHost, pPkt);
	if (pPkt.find(READ) == 0 && checkLength(pPkt, READ)) strResp = handleReadCmd(pPkt);
	if (pPkt.find(WRITE) == 0 && checkLength(pPkt, WRITE)) strResp = handleWriteCmd(pHost, pClientAddr, pPkt);
	if (pPkt.find(REPLACE) == 0 && checkLength(pPkt, REPLACE)) strResp = handleReplaceCmd(pHost, pClientAddr, pPkt);

	if (!strResp.empty()) mpNetMagr->sendPacket(pClientAddr, strResp);
	mLogger << MODULE_NAME << "onNetPacket Response : " << strResp << std::endl;
	return true;
}

void ClientManager::onTimeout(size_t pTimeoutId) { mLogger << MODULE_NAME << "Got timeout for Timer Id " << pTimeoutId << std::endl; }

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