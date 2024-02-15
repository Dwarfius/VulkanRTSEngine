#pragma once

#ifdef ASSERT_MUTEX

#include <atomic>
#include <source_location>

// A utility mutex that asserts on double write or write-while-read situations. 
// Multiple read is accepted.
class AssertRWMutex
{
public:
	class LockState
	{
	public:
		LockState(AssertRWMutex& aMutex, const std::source_location& aLoc)
			: myMutex(aMutex)
			, myLoc(aLoc)
		{
		}

		AssertRWMutex& myMutex;
		std::source_location myLoc;
	};

public:
	void LockWrite(const LockState& aLock);
	void UnlockWrite();

	void LockRead(const LockState& aLock);
	void UnlockRead();

private:
	constexpr static uint8_t kWriteVal = 1 << 7;

	std::atomic<const LockState*> myLastLock = nullptr;
	std::atomic<uint8_t> myLock = 0;
};

// A locker utility that implements RAII on AssertRWMutex for read
class AssertWriteLock : private AssertRWMutex::LockState
{
public:
	AssertWriteLock(AssertRWMutex& anRWMutex, 
		std::source_location aLocation = std::source_location::current());
	~AssertWriteLock();
};

// A locker utility that implements RAII on AssertRWMutex for read
class AssertReadLock : private AssertRWMutex::LockState
{
public:
	AssertReadLock(AssertRWMutex& anRWMutex,
		std::source_location aLocation = std::source_location::current());
	~AssertReadLock();
};

#endif // ASSERT_MUTEX