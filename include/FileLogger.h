//sgn
#pragma once

#define FILELOGGER_H_

#include <mutex>
#include <sstream>
#include <iostream>
#include <fstream>

namespace util {
	class Logger {

	public:
		virtual ~Logger() {}

		Logger& operator<<(const std::string strLog);
		Logger& operator<<(const int val);

			// this is the type of std::cout
		typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
		// this is the function signature of std::endl
		typedef CoutType& (*StandardEndLine)(CoutType&);
		Logger& operator << (StandardEndLine manip);

		static Logger& getInstance();

	private:
		std::ofstream mLogFile;
		std::mutex writeLock;
		bool mbTime;

		Logger();
		void stampTime();
		static Logger* pLogger;
	};

}	// namespace util