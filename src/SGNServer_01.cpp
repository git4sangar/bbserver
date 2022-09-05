//Sri Gurubhyo Namaha SGNServer_01.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <memory>
#include <thread>

#include "Constants.h"
#include "FileManager.h"
#include "ReadWriteLock.h"
#include "ClientManager.h"
#include "Timer.h"

using namespace util;

int main(int argc, char* argv[])
{
    FileManager::Ptr pFileMgr = std::make_shared<FileManager>();
    pFileMgr->init();
    ReadWriteLock::Ptr pRdWrLock = std::make_shared<ReadWriteLock>();

    ClientManager::Ptr pClientMgr = std::make_shared<ClientManager>(pFileMgr, pRdWrLock);
    //pClientMgr->onNetPacket("localhost", 10000, READ + std::string(" 100"));

    return 0;
}
