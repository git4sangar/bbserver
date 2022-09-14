
#include <fstream>
#include <string>
#include <vector>
#include "ConfigManager.h"

#define MODULE_NAME "ConfigManager : "

namespace util {
    ConfigManager* ConfigManager::pThis = nullptr;

    ConfigManager::ConfigManager()
        : mThreadMax {20}
        , mBBPort { 9000 }
        , mSyncPort { 10000 }
        , m_d { true }
        , m_D { false }
        , mLogger { Logger::getInstance() }
    {}

    ConfigManager* ConfigManager::getInstance() {
        if (pThis == nullptr) {
            pThis = new ConfigManager();
            pThis->parseCfgFile("bbserv.conf");
        }
        return pThis;
    }

    void ConfigManager::parseCfgFile(const std::string& pCfgFileName) {
        mLogger << MODULE_NAME << "parsing config file " << pCfgFileName << std::endl;
        if (pCfgFileName.empty()) { mLogger << MODULE_NAME << "ConfigManager: Invalid cfg file" << std::endl; return; }
        std::ifstream cfgFile(pCfgFileName);
        if (!cfgFile) { mLogger << MODULE_NAME << "ConfigManager: Error reading cfg file" << std::endl; return; }

        std::string strLine, strKey, strVal;
        mLogger << MODULE_NAME << "started parsing config file " << pCfgFileName << std::endl;
        while (std::getline(cfgFile, strLine)) {
            auto nPos = strLine.find_first_of("=");
            if (std::string::npos != nPos && (nPos+1) < strLine.length()) {
                strKey = strLine.substr(0, nPos);
                strVal = strLine.substr(nPos + 1);

                if (strKey == "THMAX") mThreadMax = std::stol(strVal);
                else if (strKey == "BBPORT") mBBPort = std::stol(strVal);
                else if (strKey == "SYNCPORT") mSyncPort = std::stol(strVal);
                else if (strKey == "BBFILE") mBBFile = strVal;
                else if (strKey == "PEERS") mPeers = parsePeers(strVal);
                else if (strKey == "DAEMON") m_d = parseBool(strVal);
                else if (strKey == "DEBUG") m_D = parseBool(strVal);
            }
            strLine.clear();
        }

        //  Logging {{{
        mLogger << MODULE_NAME << "BBPORT: " << mBBPort << ", SYNCPORT: " << mSyncPort
            << "\nBBFILE: " << mBBFile << "\nDAEMON: " << m_d << ", DEBUG: " << m_D << std::endl;
        mLogger << MODULE_NAME << "Peers : ";
        std::string strPrefix = "";
        for (const auto& [host, port] : mPeers) {
            mLogger << strPrefix << host << ":" << port;
            strPrefix = ", ";
        }
        mLogger << std::endl << std::endl;
        //  Logging }}}

        cfgFile.close();
    }

    unsigned int ConfigManager::getMaxThreads() { return mThreadMax; }
    unsigned int ConfigManager::getBPort() { return mBBPort; }
    unsigned int ConfigManager::getSyncPort() { return mSyncPort; }
    const std::string& ConfigManager::getBBFile() { return mBBFile; }
    bool ConfigManager::isServerStartup() { return m_d; }
    bool ConfigManager::isDebug() { return m_D; }
    const std::vector<std::pair<std::string, uint32_t>>& ConfigManager::getPeers() { return mPeers; }

    ConfigManager::~ConfigManager() {}

    bool ConfigManager::parseBool(const std::string& strVal) {
        if (strVal.empty()) throw std::runtime_error("ConfigManager: Invalid bool input");
        if (strVal[0] == '1' || strVal == "true") return true;
        if (strVal[0] == '0' || strVal == "false") return false;
        return false;
    }

    std::vector<std::pair<std::string, uint32_t>> ConfigManager::parsePeers(const std::string& strVal) {
        std::vector<std::pair<std::string, uint32_t>> retPairs;
        if (strVal.empty()) return retPairs;

        //  Get the pairs
        std::vector<std::string> ipPortPairs;
        std::string ipPortPair;
        for(const auto& ch : strVal) {
            if (ch != ' ') { ipPortPair += ch; }
            else if (!ipPortPair.empty()) {
                ipPortPairs.push_back(ipPortPair);
                ipPortPair.clear();
            }
        }
        if (!ipPortPair.empty()) ipPortPairs.push_back(ipPortPair);

        //  Split ip and port
        for (const auto& ipPortPair : ipPortPairs) {
            auto nPos = ipPortPair.find_first_of(":");
            if (nPos != std::string::npos && (nPos + 1) < ipPortPair.length()) {
                auto strKey = ipPortPair.substr(0, nPos);
                auto iPort = std::stol(ipPortPair.substr(nPos+1));
                retPairs.push_back( std::make_pair(strKey, iPort) );
            }
        }

        return retPairs;
    }

}	// namespace util
