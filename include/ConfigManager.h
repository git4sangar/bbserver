#pragma once

#include <iostream>
#include <map>

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

		const std::map<std::string, unsigned int>& getPeers();

	private:
		ConfigManager();
		void parseConfig(const std::string& pCfgFileName);
		std::map<std::string, unsigned int> parsePeers(const std::string& strVal);
		bool parseBool(const std::string& strVal);

		unsigned int mThreadMax, mBBPort, mSyncPort;
		std::string mBBFile;
		bool m_d, m_D;
		std::map<std::string, unsigned int> mPeers;

		static ConfigManager* pThis;
	};
}

