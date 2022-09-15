//sgn
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <regex>
#include <sys/stat.h>

#include "FileManager.h"

#define MODULE_NAME "FileManager : "

namespace util {
	FileManager::FileManager()
		: mpCfgMgr{ ConfigManager::getInstance() }
		, mLastMsgNo{ 0 }
		, mLastWrittenPos{ 0 }
		, mLastWrittenLen{ 0 }
		, mLogger{ Logger::getInstance() }
	{
		//	Create BBFile if it does not exists already
		struct stat buffer;
		if (stat(mpCfgMgr->getBBFile().c_str(), &buffer) != 0) {
			std::ofstream fs(mpCfgMgr->getBBFile()); fs.close();
		}
		mBBFile.open(mpCfgMgr->getBBFile(), std::ios::out | std::ios::in);
	}

	void FileManager::init() {
		//	Populate File Indices with message no, start pos and length
		std::string strLine;
		size_t ulLineStart = 0;
		mBBFile.seekg(0, std::ios::beg);

		unsigned int uiNoOfLines = 0;
		mLogger << MODULE_NAME << "Initializing" << std::endl;
		while (getLine(mBBFile, strLine, ulLineStart)) {
			strLine = std::regex_replace(strLine, std::regex("^\\s+"), std::string(""));
			strLine = std::regex_replace(strLine, std::regex("\\s+$"), std::string(""));
			if (strLine.empty()) continue;

			auto nPos = strLine.find_first_of("/");
			if (nPos == std::string::npos) {
				mLogger << MODULE_NAME << "File corrupted, throwing exception" << std::endl;
				throw std::runtime_error("FileManager: Corrupted BBFile");
			}

			auto strMsgNo = strLine.substr(0, nPos);
			auto ulMsgNo = std::stoul(strMsgNo);
			if (ulMsgNo == 0) continue;
			if (ulMsgNo > mLastMsgNo) mLastMsgNo = ulMsgNo;

			size_t ulLen = strLine.length();
			auto posAndLen = std::make_pair(ulLineStart, ulLen + 1); // include \n len

			mFileIndices.insert({ulMsgNo, posAndLen});
			uiNoOfLines++;
		}
		mLogger << MODULE_NAME << "Parsed " << uiNoOfLines << " lines" << std::endl;
	}

	std::string FileManager::readFromFile(size_t pMsgNo) {
		std::string strLine;

		if (mFileIndices.count(pMsgNo) > 0) {
			auto posAndLen = mFileIndices.at(pMsgNo);
			char* pBuffer = new char[posAndLen.second + 1];

			mAccessLock.lock();
			mBBFile.clear();
			mBBFile.seekg(posAndLen.first, std::ios::beg);
			mBBFile.read(pBuffer, posAndLen.second);
			mAccessLock.unlock();

			pBuffer[posAndLen.second-1] = '\0'; // omit \n while reading
			strLine = pBuffer;
			delete[] pBuffer;

			mLogger << MODULE_NAME << "Pos " << posAndLen.first << ", Len " << posAndLen.second << ", Line " << strLine << std::endl;
		} else {
			mLogger << MODULE_NAME << "Read failed as Msg no " << pMsgNo << " does not exist" << std::endl;
		}
		
		return strLine;
	}

	size_t FileManager::writeOrReplace(const std::string& pSender, const std::string& pMsg, size_t pReplaceNo) {
		if(pReplaceNo > 0) return replaceMessage(pReplaceNo, pSender, pMsg);

		std::stringstream ss;
		ss << ++mLastMsgNo << "/" << pSender << "/" << pMsg << std::endl;
		if (ss.str().length() < MIN_MSG_LENGTH) return false;

		size_t ulLineStart = moveToEOF();
		writeAtPos(0, std::ios::end, ss.str());

		mLastWrittenPos = ulLineStart;
		mLastWrittenLen = ss.str().length();
		auto posAndLen = std::make_pair(ulLineStart, ss.str().length());
		mFileIndices.insert({ mLastMsgNo, posAndLen });

		mLogger << MODULE_NAME << "Wrote msg no " << mLastMsgNo << " with content " << ss.str() << " and len " << ss.str().length() << std::endl;
		return mLastMsgNo;
	}

	bool FileManager::undoLastWritten() {
		if (mLastWrittenLen < MIN_MSG_LENGTH) return false;
		mLogger << MODULE_NAME << "Undo last written msg" << std::endl;
		writeDummyAtPos(mLastWrittenPos, mLastWrittenLen);
		mFileIndices.erase(mLastMsgNo);
		mLastMsgNo--;

		return true;
	}

	size_t FileManager::replaceMessage(size_t pMsgNo, const std::string& pSender, const std::string& pMsg) { 
		if (mFileIndices.count(pMsgNo) == 0) return 0;

		std::stringstream ss;
		ss << pMsgNo << "/" << pSender << "/" << pMsg << std::endl;
		std::pair<size_t,size_t> posAndLen = mFileIndices.at(pMsgNo);

		if (ss.str().length() == posAndLen.second) {
			mLogger << MODULE_NAME << "Replace : Exising and New Line are equal in length" << std::endl;
			writeAtPos(posAndLen.first, std::ios::beg, ss.str());
		} /*else if (ss.str().length() < posAndLen.second) {
			mLogger << MODULE_NAME << "Replace : New Line < Existing Line" << std::endl;
			std::string strPad(posAndLen.second - ss.str().length(), ' ');
			ss.str(""); ss << pMsgNo << "/" << pSender << "/" << pMsg << strPad << std::endl;
			writeAtPos(posAndLen.first, std::ios::beg, ss.str());
			mFileIndices[pMsgNo].second = ss.str().length();
		}*/ else {
			mLogger << MODULE_NAME << "Replace : Existing Line != New Line" << std::endl;
			writeDummyAtPos(posAndLen.first, posAndLen.second);
			size_t ulLineStart = moveToEOF();
			writeAtPos(ulLineStart, std::ios::beg, ss.str());
			mFileIndices[pMsgNo].first = ulLineStart;
			mFileIndices[pMsgNo].second = ss.str().length();
		}
		return pMsgNo;
	}

	bool FileManager::getLine(std::fstream& pStream, std::string& pStrLine, size_t& pPos) {
		char ch;
		bool iRet = false;
		pPos = pStream.tellg();
		pStrLine.clear();
		while (pStream.get(ch)) {
			if (ch == '\n' || ch == '\r') { iRet = true; break; }
			pStrLine += ch;
		}
		return iRet || !pStrLine.empty();
	}

	void FileManager::writeAtPos(size_t pPos, std::ios::seekdir pDir, const std::string& pMsg) {
		mBBFile.clear();
		mBBFile.seekp(pPos, pDir);
		mBBFile.write(pMsg.c_str(), pMsg.length());
		mBBFile.flush();
	}

	void FileManager::writeDummyAtPos(size_t pPos, size_t pLen) {
		if (pLen > MIN_MSG_LENGTH) {
			std::string strDummy(pLen-1, '0'); // don't overwirte the \n
			strDummy[1] = '/';
			strDummy[3] = '/';
			writeAtPos(pPos, std::ios::beg, strDummy);
		}
	}

	size_t FileManager::moveToEOF() {
		mBBFile.clear();
		mBBFile.seekg(0, std::ios::end);
		return mBBFile.tellg();
	}

	void FileManager::close() {
		mLastMsgNo = mLastWrittenPos = mLastWrittenLen = 0;
		mBBFile.close();
		mFileIndices.clear();
		mLogger << MODULE_NAME << "FileManager closed" << std::endl;
	}
} // namepsace util
