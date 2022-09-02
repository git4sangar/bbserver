//sgn

#include <thread>
#include <chrono>
#include <vector>
#include "Timer.h"

namespace util {
	Timer* Timer::pThis = nullptr;

	size_t Timer::pushToTimerQueue(TimerListener::Ptr pListener, size_t pDurationSecs) {
		if (!pListener || !pDurationSecs) return 0;
		TimerElement::Ptr pTimerElement = std::make_shared<TimerElement>();

		pTimerElement->mListener = pListener;
		pTimerElement->mDuration = pDurationSecs;

		mQueueLock.lock();
		pTimerElement->mTimerId = ++mNextId;
		mTimerElements.push_back(pTimerElement);
		mQueueLock.unlock();

		return pTimerElement->mTimerId;
	}

	bool Timer::removeFromTimerQueue(size_t pTimerId) {
		bool bFlag = false;
		mQueueLock.lock();
		for (auto pElementItr = mTimerElements.begin(); pElementItr != mTimerElements.end(); ) {
			TimerElement::Ptr pElement = *pElementItr;
			if (pElement->mTimerId == pTimerId) {
				bFlag = true;
				pElementItr = mTimerElements.erase(pElementItr);
				break;
			}
			else pElementItr++;
		}
		mQueueLock.unlock();
		return bFlag;
	}

	void Timer::run() {
		std::vector<TimerElement::Ptr> toCallList;

		while (1) {
			toCallList.clear();

			mQueueLock.lock();
			for (auto pElementItr = mTimerElements.begin(); pElementItr != mTimerElements.end(); ) {
				TimerElement::Ptr pElement = *pElementItr;
				pElement->mDuration--;
				if (pElement->mDuration == 0) {
					pElementItr = mTimerElements.erase(pElementItr);
					toCallList.push_back(pElement);
				}
				else pElementItr++;
			}
			mQueueLock.unlock();

			for (const auto& pElement : toCallList) {
				//	Submit the following to threadpool
				pElement->mListener->onTimeout(pElement->mTimerId);
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}	// namespace util
