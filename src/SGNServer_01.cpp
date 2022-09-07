//Sri Gurubhyo Namaha SGNServer_01.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <exception>

#include "Constants.h"
#include "FileManager.h"
#include "ReadWriteLock.h"
#include "ClientManager.h"
#include "FileLogger.h"

using namespace util;

void printHelp() {
    std::cout << "-c name of the config file to be used" << std::endl
              << "-b name of the bbfile" << std::endl
              << "-T max no of threas in threadpool" << std::endl
              << "-p port number to which user communicates" << std::endl
              << "-s port number to which peers communicate" << std::endl
              << "-f toggles \"server start steps\"" << std::endl
              << "-d toggles debug output" << std::endl;
}

int main(int argc, char* argv[])
{
    util::ConfigManager *pCfgMgr = nullptr;
    try { pCfgMgr = ConfigManager::getInstance(); }
    catch(std::exception &e) { std::cout << e.what() << std::endl; }

    bool bDebug = false, bStartUp = false;
    int opt = 0;
    while ((opt = getopt(argc, argv, "c:b:T:p:s:fdh")) != -1) {
        switch(opt) {
            case 'c':
                try {pCfgMgr->parseCfgFile(optarg);
                    if(pCfgMgr->getBBFile().empty()) throw std::runtime_error("Missing BBFile");}
                catch(std::exception &e) {
                    std::cout << e.what() << std::endl;
                    return 0;
                }
                break;
            case 'b':
                pCfgMgr->setBBFile(optarg);
                break;
            case 'T':
                try {pCfgMgr->setMaxThreads(std::stol(optarg));}
                catch(std::exception &e) {std::cout << e.what() << std::endl;}
                break;
            case 'p':
                try {pCfgMgr->setBPort(std::stol(optarg));}
                catch(std::exception &e) {std::cout << e.what() << std::endl;}
                break;
            case 's':
                try {pCfgMgr->setSyncPort(std::stol(optarg));}
                catch(std::exception &e) {std::cout << e.what() << std::endl;}
                break;
            case 'f':
                bStartUp = !bStartUp;
                pCfgMgr->setServerStartup(bStartUp);
                break;
            case 'd':
                bDebug = !bDebug;
                pCfgMgr->setDebug(bDebug);
                break;
            default:
            case 'h':
                printHelp();
                break;            
        }
    }

    ReadWriteLock::Ptr pRdWrLock = std::make_shared<ReadWriteLock>();    
    FileManager::Ptr pFileMgr = std::make_shared<FileManager>();
    pFileMgr->init();

    ClientManager::Ptr pClientMgr = std::make_shared<ClientManager>(pFileMgr, pRdWrLock);
    pClientMgr->init();
    sleep(1);   //  Wait a bit for all modules to initialize and get ready

    Logger::getInstance().getInstance() << "--- Started BBServer Successfully ---" << std::endl << std::endl;
    while(1) sleep(60);
    return 0;
}
