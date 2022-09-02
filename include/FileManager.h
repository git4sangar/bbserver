//SGN 
#pragma once

#include <iostream>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <mutex>
#include <utility>

#include "ConfigManager.h"
#include "FileLogger.h"

#define MIN_MSG_LENGTH (6)	// Eg: 0/0/0
namespace util {
	class FileManager
	{
	public:
		typedef std::shared_ptr<FileManager> Ptr;

		FileManager();
		void init();
		~FileManager() { mBBFile.close(); }

		std::string readFromFile(size_t pMsgNo);
		size_t writeToFile(std::string pSender, std::string pMsg);
		bool replaceMessage(size_t pMsgNo, std::string pSender, std::string pMsg);
		bool undoLastWritten();

	private:
		bool getLine(std::fstream& pStream, std::string& pStrLine, size_t& pPos);

		void writeAtPos(size_t pPos, std::ios::seekdir pDir, const std::string& pMsg);
		void writeDummyAtPos(size_t pPos, size_t pLen);
		size_t moveToEOF();

		std::unordered_map<size_t, std::pair<size_t, size_t>> mFileIndices;
		ConfigManager* mpCfgMgr;
		std::fstream mBBFile;
		std::mutex mAccessLock;
		size_t mLastMsgNo, mLastWrittenPos, mLastWrittenLen;
		Logger& mLogger;
	};

} // namespace util