#pragma once

#include <iostream>
#include <map>
#include <utility>
#include <vector>

#include "FileLogger.h"

namespace util {
	class ConfigManager
	{
	public:
		~ConfigManager();
		static ConfigManager* getInstance();

		unsigned int getMaxThreads();
		unsigned int getBPort();
		unsigned int getSyncPort();
		const std::string& getBBFile();
		bool isServerStartup();
		bool isDebug();

		const std::vector<std::pair<std::string, uint32_t>>& getPeers();

		void parseCfgFile(const std::string& pCfgFileName);
		void setMaxThreads(unsigned int pMaxThreads) { mThreadMax = pMaxThreads; }
		void setBPort(unsigned int pBPort) {mBBPort = pBPort; }
		void setSyncPort(unsigned int pSPort) { mSyncPort = pSPort; }
		void setBBFile(std::string pBBFile) { mBBFile = pBBFile; }
		void setServerStartup(bool isTrue) {m_d = isTrue;}
		void setDebug(bool isTrue) { m_D = isTrue; }
		void setPeers(const std::vector<std::pair<std::string, uint32_t>>& pPeers) { mPeers = pPeers; }
		std::vector<std::pair<std::string, uint32_t>> parsePeers(const std::string& strVal);

	private:
		ConfigManager();
		bool parseBool(const std::string& strVal);

		unsigned int mThreadMax, mBBPort, mSyncPort;
		std::string mBBFile;
		bool m_d, m_D;
		std::vector<std::pair<std::string, uint32_t>> mPeers;
		Logger& mLogger;

		static ConfigManager* pThis;
	};
}

