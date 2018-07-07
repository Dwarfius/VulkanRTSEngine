#pragma once

#ifdef ASSERT_MUTEX

#include <atomic>
#include <thread>

// A utility mutex that asserts on double write or write-while-read situations. 
// Multiple read is accepted.
class AssertRWMutex
{
public:
	AssertRWMutex();

	void LockWrite();
	void UnlockWrite();

	void LockRead();
	void UnlockRead();

private:
	atomic<unsigned char> myLock;
	thread::id myLastReadThreadId; // for debug info
	thread::id myWriteThreadId; // for debug info
};

// A locker utility that implements RAII on AssertRWMutex for read
class AssertWriteLock
{
public:
	AssertWriteLock(AssertRWMutex& anRWMutex);
	~AssertWriteLock();

private:
	AssertRWMutex & myMutex;
};

// A locker utility that implements RAII on AssertRWMutex for read
class AssertReadLock
{
public:
	AssertReadLock(AssertRWMutex& anRWMutex);
	~AssertReadLock();

private:
	AssertRWMutex& myMutex;
};

#endif // ASSERT_MUTEX