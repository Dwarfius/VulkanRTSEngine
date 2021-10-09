#pragma once

#ifdef ASSERT_MUTEX

#include <atomic>
#include <thread>
#include <source_location>

// A utility mutex that asserts on double write or write-while-read situations. 
// Multiple read is accepted.
class AssertRWMutex
{
public:
	void LockWrite(std::source_location aLocation);
	void UnlockWrite();

	void LockRead(std::source_location aLocation);
	void UnlockRead();

private:
	constexpr static uint8_t kWriteVal = 1 << 7;

	std::atomic<uint8_t> myLock = 0;
	std::atomic<uint_least32_t> myWriteLine = 0;
	std::atomic<const char*> myWriteFuncName = "";
	std::atomic<const char*> myWriteFileName = "";
	std::atomic<uint_least32_t> myReadLine = 0;
	std::atomic<const char*> myReadFuncName = "";
	std::atomic<const char*> myReadFileName = "";
};

// A locker utility that implements RAII on AssertRWMutex for read
class AssertWriteLock
{
public:
	AssertWriteLock(AssertRWMutex& anRWMutex, 
		std::source_location aLocation = std::source_location::current());
	~AssertWriteLock();

private:
	AssertRWMutex& myMutex;
};

// A locker utility that implements RAII on AssertRWMutex for read
class AssertReadLock
{
public:
	AssertReadLock(AssertRWMutex& anRWMutex,
		std::source_location aLocation = std::source_location::current());
	~AssertReadLock();

private:
	AssertRWMutex& myMutex;
};

#endif // ASSERT_MUTEX