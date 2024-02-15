#include "Precomp.h"
#include "AssertRWMutex.h"

#ifdef ASSERT_MUTEX

#include <sstream>

void AssertRWMutex::LockWrite(const LockState& aLock)
{
	// we set these first so that they are available in case of detected contention
	const LockState* lastState = myLastLock.exchange(&aLock, std::memory_order::relaxed);
	std::atomic_thread_fence(std::memory_order::acq_rel);
	
	// most-significant bit is reserved for a write lock
	// check whether we just locked a write-locked mutex
	unsigned char currVal = myLock.fetch_or(kWriteVal);
	// if any of the bits are set, then a reader write/read-lock is active
	ASSERT_STR(currVal == 0, "Contention! {} at {}:{} tried to lock mutex for write "
		"while it's locked for read by {} at {}:{}",
		aLock.myLoc.function_name(), aLock.myLoc.file_name(), aLock.myLoc.line(),
		lastState->myLoc.function_name(), lastState->myLoc.file_name(), lastState->myLoc.line()
	);
}

void AssertRWMutex::UnlockWrite()
{
	myLock.fetch_xor(kWriteVal);
	{
		myLastLock.store(nullptr, std::memory_order::relaxed);
		std::atomic_thread_fence(std::memory_order::release);
	}
}

void AssertRWMutex::LockRead(const LockState& aLock)
{
	// we set these first  so that they are available in case of detected contention
	const LockState* lastState = myLastLock.exchange(&aLock, std::memory_order::relaxed);
	std::atomic_thread_fence(std::memory_order::acq_rel);
	// check whether we've just read-locked a write-locked mutex
	unsigned char currVal = myLock.fetch_add(1);
	ASSERT_STR((currVal & kWriteVal) == 0, "Contention! {} at {}:{} tried to lock mutex for read "
		"while it's locked for write by {} at {}:{}",
		aLock.myLoc.function_name(), aLock.myLoc.file_name(), aLock.myLoc.line(),
		lastState->myLoc.function_name(), lastState->myLoc.file_name(), lastState->myLoc.line()
	);
}

void AssertRWMutex::UnlockRead()
{
	const uint8_t oldVal = myLock.fetch_sub(1);

	// We only reset the last location if there are no more reads
	if(oldVal == 1)
	{
		myLastLock.store(nullptr, std::memory_order::relaxed);
		std::atomic_thread_fence(std::memory_order::release);
	}
}

// ========================

AssertWriteLock::AssertWriteLock(AssertRWMutex& anRWMutex, std::source_location aLocation)
	: AssertRWMutex::LockState(anRWMutex, aLocation)
{
	myMutex.LockWrite(*this);
}

AssertWriteLock::~AssertWriteLock()
{
	myMutex.UnlockWrite();
}

// ========================

AssertReadLock::AssertReadLock(AssertRWMutex& anRWMutex, std::source_location aLocation)
	: AssertRWMutex::LockState(anRWMutex, aLocation)
{
	myMutex.LockRead(*this);
}

AssertReadLock::~AssertReadLock()
{
	myMutex.UnlockRead();
}

#endif // ASSERT_MUTEX