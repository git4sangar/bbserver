//Sri Gurubhyo Namaha SGNServer_01.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <exception>
#include <signal.h>
#include <string.h>

#include "Constants.h"
#include "FileLogger.h"
#include "FileManager.h"
#include "ReadWriteLock.h"
#include "ClientManager.h"
#include "FileLogger.h"
#include "ThreadPool.h"

using namespace util;

//  Global pointers as we need to access in signal handler
ReadWriteLock::Ptr pRdWrLock = nullptr;
FileManager::Ptr pFileMgr = nullptr;
ClientManager::Ptr pClientMgr = nullptr;
ThreadPool::Ptr pThreadPool = nullptr;
Logger& logger = Logger::getInstance();

void printHelp() {
    logger << "BBServer 1.0" << std::endl
              << "Usage: BBServer [option] as follows" << std::endl
              << "-c followed by name of the config file to be used" << std::endl
              << "-b followed by name of the bbfile" << std::endl
              << "-T followed by max no of threas in threadpool" << std::endl
              << "-p followed by port number to which user communicates" << std::endl
              << "-s followed by port number to which peers communicate" << std::endl
              << "-f (no argument) toggles \"server start steps\"" << std::endl
              << "-d (no argument) toggles debug output" << std::endl
              << "Handles signals: SIGQUIT, SIGUP\nEg: kill -s SIGQUIT PID" << std::endl;
}

void startApp() {
    ConfigManager *pCfgMgr = ConfigManager::getInstance();
    pThreadPool = std::make_shared<ThreadPool>(pCfgMgr->getMaxThreads());

    pRdWrLock = std::make_shared<ReadWriteLock>();    
    pFileMgr = std::make_shared<FileManager>();
    pFileMgr->init();

    pClientMgr = std::make_shared<ClientManager>(pFileMgr, pRdWrLock);
    pClientMgr->init();
    sleep(1);   //  Wait a bit for all modules to initialize and get ready
    logger << "--- Started BBServer Successfully ---" << std::endl << std::endl;
}

void sighupHandler(int signum) { pClientMgr->onSigHup(); startApp(); }
void sigQuitHandler(int signum) { pClientMgr->onSigHup(); exit(0); }

int main(int argc, char* argv[]) {
    if(argc == 2 && !strcmp(argv[1], "-h")) { printHelp(); return 0; }

    //  Parse the config file
    ConfigManager *pCfgMgr = nullptr;
    try { pCfgMgr = ConfigManager::getInstance(); }
    catch(std::exception &e) { logger << e.what() << std::endl; }

    //  Handle command line arguments and override parsed cfg file
    bool bDebug = false, bStartUp = false;
    int opt = 0;
    while ((opt = getopt(argc, argv, "c:b:T:p:s:fdh")) != -1) {
        switch(opt) {
            case 'c':
                pCfgMgr->parseCfgFile(optarg);
                break;
            case 'b':
                pCfgMgr->setBBFile(optarg);
                break;
            case 'T':
                try {pCfgMgr->setMaxThreads(std::stol(optarg));}
                catch(std::exception &e) {logger << e.what() << std::endl;}
                break;
            case 'p':
                try {pCfgMgr->setBPort(std::stol(optarg));}
                catch(std::exception &e) {logger << e.what() << std::endl;}
                break;
            case 's':
                try {pCfgMgr->setSyncPort(std::stol(optarg));}
                catch(std::exception &e) {logger << e.what() << std::endl;}
                break;
            case 'f':
                bStartUp = !bStartUp;
                pCfgMgr->setServerStartup(bStartUp);
                break;
            case 'd':
                bDebug = !bDebug;
                pCfgMgr->setDebug(bDebug);
                break;          
        }
    }
    //  Parse non-switch arguments
    if(argc - optind > 0) {
        std::string strArgs;
        for(uint32_t iLoop = optind; iLoop < argc; iLoop++) strArgs += (std::string(argv[iLoop]) + ' ');

        const auto& peers = pCfgMgr->parsePeers(strArgs);
        pCfgMgr->setPeers(peers);
    }

    try { if(pCfgMgr->getBBFile().empty()) throw std::runtime_error("Missing BBFile");}
    catch(std::exception &e) { logger << e.what() << std::endl; return 0; }

    pid_t pid = fork();
    if(0 < pid) exit(EXIT_SUCCESS); // parent process
    if(0 > pid) return -1;  //  error creating child process

    //  Child Process
    signal(SIGHUP,sighupHandler);
    signal(SIGQUIT,sigQuitHandler);

    //  Write the pid to file first
    pid_t myPid = ::getpid();
    logger << "My PID " << myPid << std::endl;
    std::ofstream pidFile("bbserv.pid", std::ios::out | std::ios::trunc);
    pidFile << myPid;
    pidFile.close();

    startApp(); //  Start the app

    while(1) sleep(60); //  Just sleep in background
    return 0;
}
