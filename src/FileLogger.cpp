//sgn

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "Constants.h"
#include "FileLogger.h"

#define MODULE_NAME "Logger : "

namespace util {
	Logger* Logger::pLogger = NULL;

	Logger& Logger::getInstance() {
		if (NULL == pLogger) {
			pLogger = new Logger();
			*pLogger << MODULE_NAME << "Logger Started" << std::endl;
		}
		return *pLogger;
	}

	Logger::Logger() : mbTime{ true } {
		mLogFile.open("bbserv.log", std::ios::out);
	}

#if __linux__
	void Logger::stampTime() {
		/*struct timeval st;
		gettimeofday(&st, NULL);

		//	why do i need secs since epoch? get secs from now
		//	1584718500 => secs since epoch till now (20-Mar-20 21:05)
		unsigned long secs = st.tv_sec - 1584718500;
		secs = secs % 36000;	// reset secs every 10 hours
		unsigned long msecs = st.tv_usec / 1000;
		unsigned long usecs = st.tv_usec % 1000;
		std::cout << secs << ":" << msecs << ":" << usecs << ": ";*/
	}
#else
	void Logger::stampTime() {
		std::cout << 4 << ":" << 4 << ":" << 4 << ": ";
		mLogFile << 4 << ":" << 4 << ":" << 4 << ": ";
		mLogFile.flush();
	}
#endif

	Logger& Logger::operator << (StandardEndLine manip) {
		writeLock.lock();
		mLogFile << std::endl; mLogFile.flush();
		std::cout << std::endl;

		writeLock.unlock();
		return *this;
	}

	Logger& Logger::operator <<(const std::string& strMsg) {
		writeLock.lock();
		if (mbTime) { stampTime(); mbTime = false; }
		mLogFile << strMsg; mLogFile.flush();
		std::cout << strMsg;
		writeLock.unlock();
		return *this;
	}

	Logger& Logger::operator <<(const size_t iVal) {
		writeLock.lock();
		if (mbTime) { stampTime(); mbTime = false; }
		mLogFile << iVal; mLogFile.flush();
		std::cout << iVal;
		writeLock.unlock();
		return *this;
	}

}	// namespace util
