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
    pid_t pid;
    pid = fork();

    //  Exit the parent process
    if(0 < pid) {
        std::cout << "Exiting pid " << getpid() << std::endl;
        exit(EXIT_SUCCESS);
    }

    //  -ve return means error
    if(0 > pid) {
        std::cout << "Error creating child process" << std::endl;
        return -1;
    }

    //--------------Child Process--------------
    signal(SIGHUP,sighupHandler);
    signal(SIGQUIT,sigQuitHandler);

    if(argc == 2 && !strcmp(argv[1], "-h")) { printHelp(); return 0; }

    logger << "My PID " << ::getpid() << std::endl;
    ConfigManager *pCfgMgr = nullptr;
    try { pCfgMgr = ConfigManager::getInstance(); }
    catch(std::exception &e) { logger << e.what() << std::endl; }

    bool bDebug = false, bStartUp = false;
    int opt = 0;
    while ((opt = getopt(argc, argv, "c:b:T:p:s:fdh")) != -1) {
        switch(opt) {
            case 'c':
                try {pCfgMgr->parseCfgFile(optarg);
                    if(pCfgMgr->getBBFile().empty()) throw std::runtime_error("Missing BBFile");}
                catch(std::exception &e) {
                    logger << e.what() << std::endl;
                    return 0;
                }
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

    startApp();

    while(1) sleep(60);
    return 0;
}
