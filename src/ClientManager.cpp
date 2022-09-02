//sgn
#include <iostream>
#include <regex>
#include <sstream>

#include "Constants.h"
#include "ClientManager.h"

std::string ClientManager::handleUserCmd(const std::string& pHost, const std::string pPkt) {
	std::stringstream ss;

	if (pPkt.find_first_not_of(ALPHA_CAPS + ALPHA_SMALL + NUMERIC + "\t ") != std::string::npos) {
		return "ERROR USER Invalid text";
	}
	if (mHost2UserMap.count(pHost) > 0) {
		ss.str(""); ss << "HELLO " << mHost2UserMap[pHost] << ", how are you?";
		return ss.str();
	}
	if (pPkt.length() > USER.length()) {
		std::string userName;
		userName = pPkt.substr(USER.length());
		userName = myTrim(userName);
		mHost2UserMap.insert({ pHost, userName });

		ss.str(""); ss << "HELLO " << userName << ", how are you?";
		return ss.str();
	}
	return "ERROR USER Invalid text";
}

std::string ClientManager::handleReadCmd(const std::string& pPkt) {
	if (pPkt.length() < READ.length())
		return "ERROR READ Invalid message number";

	std::string strMsgNo, strMsg;

	strMsgNo = pPkt.substr(READ.length());
	strMsgNo = myTrim(strMsgNo);
	size_t ulMsgNo = std::stoul(strMsgNo);

	mpRdWrLock->read_lock();
	strMsg = mpFileMgr->readFromFile(ulMsgNo);
	mpRdWrLock->read_unlock();

	if (strMsg.empty())
		return std::string("UNKNOWN ") + strMsgNo + " message does not exist";

	size_t pos = strMsg.find_first_of("/");
	if (pos == std::string::npos)
		return "ERROR READ Corrupted Entry";
	strMsg[pos] = ' ';

	return std::string("MESSAGE ") + strMsg;
}

std::string ClientManager::handleWriteCmd(const std::string& pHost, unsigned int pPort, const std::string& pPkt) {
	std::string strSender = "nobody";
	if (mHost2UserMap.count(pHost) > 0) strSender = mHost2UserMap[pHost];
	bool isOk = mpProtocol->onWriteRequest(pHost, pPort, strSender, pPkt);
	if (isOk)
		return std::string();

	return "ERROR WRITE another write in progress";
}

void ClientManager::onNetPacket(const std::string& pHost, unsigned int pPort, const std::string& pPkt) {
	std::string strResp;

	if (pPkt.find(USER) == 0) strResp = handleUserCmd(pHost, pPkt);
	if (pPkt.find(READ) == 0) strResp = handleReadCmd(pPkt);
	if (pPkt.find(WRITE) == 0) strResp = handleWriteCmd(pHost, pPort, pPkt);

	if (!strResp.empty()) mpNetMagr->sendPacket(pHost, pPort, strResp);
}

void ClientManager::onTimeout(size_t pTimeoutId) {}

std::string ClientManager::myTrim(std::string str) {
	str = std::regex_replace(str, std::regex("^\\s+"), std::string(""));
	str = std::regex_replace(str, std::regex("\\s+$"), std::string(""));
	return str;
}

