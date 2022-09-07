//sgn
#pragma once

#include <iostream>
#include <memory>
#include <list>
#include <utility>
#include <mutex>
#include <condition_variable>
#include "FileLogger.h"

namespace util {
	class TimerListener {
	public:
		typedef std::shared_ptr<TimerListener> Ptr;
		TimerListener() {}
		virtual ~TimerListener() {}

		virtual void onTimeout(size_t pTimeoutId) = 0;
	};

	struct TimerElement {
		typedef std::shared_ptr<TimerElement> Ptr;
		TimerElement() 
			: mListener{nullptr}
			, mDuration {0}
			, mTimerId{ 0 }
		{}
		TimerListener::Ptr mListener;
		size_t mDuration, mTimerId;
	};

	class Timer
	{
	public:
		static Timer* getInstance() {
			if (!pThis) {
				pThis = new Timer();
				std::thread tTimer(&Timer::run, pThis);
				tTimer.detach();
			}
			return pThis;
		}

		size_t pushToTimerQueue(TimerListener::Ptr pListener, size_t pDurationSecs);
		bool removeFromTimerQueue(size_t pTimerId);
		void clearAllPendingTimers() {
			mQueueLock.lock();
			mTimerElements.clear();
			mQueueLock.unlock();
		}
		void run();

	private:
		Timer() : mNextId{ 0 }, mLogger{ Logger::getInstance() } {}

		std::list<TimerElement::Ptr> mTimerElements;
		std::mutex mQueueLock;
		std::condition_variable mCondVar;
		size_t mNextId;
		Logger& mLogger;

		static Timer* pThis;
	};

}	// namespace util