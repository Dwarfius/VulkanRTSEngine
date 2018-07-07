#pragma once

#ifdef ASSERT_MUTEX

#include <atomic>
#include <thread>

// A utility mutex that asserts on double-lock
class AssertMutex
{
public:
	AssertMutex();

	void Lock();
	void Unlock();

private:
	atomic<thread::id> myWriter;
};

// A locker utility that implements RAII on AssertMutex
class AssertLock
{
public:
	AssertLock(AssertMutex& aMutex);
	~AssertLock();

private:
	AssertMutex& myMutex;
};

#endif // ASSERT_MUTEX