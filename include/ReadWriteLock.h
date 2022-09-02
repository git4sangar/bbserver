//SGN
#pragma once

#include <mutex>

namespace util {
	class ReadWriteLock
	{
		//	mReadCounter < 0 means, it is in idle state
		//	mReadCounter == 0 means, all reads are done
		//	mReadCounter > 0 means, a read is happening
	public:
		typedef std::shared_ptr<ReadWriteLock> Ptr;
		ReadWriteLock() : mReadCounter{ -1 } {}
		~ReadWriteLock() {}

		void read_lock() {
			mReadCounterLock.lock();
			(mReadCounter < 0) ? mReadCounter = 1 : mReadCounter++;

			//	Write-lock will happen only when mReadCounter is in idle state
			if (mReadCounter == 1)
				mWriteLock.lock();
			mReadCounterLock.unlock();
		}

		//	Simply calling read_unlock multiple times does not hurt
		void read_unlock() {
			mReadCounterLock.lock();
			if (mReadCounter > 0) mReadCounter--;

			//	Write-unlock will happen only when mReadCounter is 0, meaning all read ops are over
			if (mReadCounter == 0) {
				mReadCounter = -1;
				mWriteLock.unlock();
			}
			mReadCounterLock.unlock();
		}

		void write_lock() {
			mWriteLock.lock();
		}

		void write_unlock() {
			mWriteLock.unlock();
		}
	private:
		int mReadCounter;
		std::mutex mReadCounterLock, mWriteLock;
	};

}	// namespace util
