#pragma once

#ifdef ASSERT_MUTEX

#include <atomic>
#include <thread>
#include <source_location>

// A utility mutex that asserts on double-lock
class AssertMutex
{
public:
	void Lock(std::source_location aLocation);
	void Unlock();

private:
	std::atomic_flag myIsLocked;
	std::atomic<uint_least32_t> myLine = 0;
	std::atomic<const char*> myFuncName = "";
	std::atomic<const char*> myFileName = "";
};

// A locker utility that implements RAII on AssertMutex
class AssertLock
{
public:
	AssertLock(AssertMutex& aMutex, std::source_location aLocation = std::source_location::current());
	~AssertLock();

private:
	AssertMutex& myMutex;
};

#endif // ASSERT_MUTEX