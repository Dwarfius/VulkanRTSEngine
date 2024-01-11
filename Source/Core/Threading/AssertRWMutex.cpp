#include "Precomp.h"
#include "AssertRWMutex.h"

#ifdef ASSERT_MUTEX

#include <sstream>

void AssertRWMutex::LockWrite(std::source_location aLocation)
{
	// we set these first so that they are available in case of detected contention
	myWriteFuncName.store(aLocation.function_name(), std::memory_order::relaxed);
	myWriteFileName.store(aLocation.file_name(), std::memory_order::relaxed);
	myWriteLine.store(aLocation.line(), std::memory_order::relaxed);
	std::atomic_thread_fence(std::memory_order::acq_rel);
	const char* funcName = myReadFuncName.load(std::memory_order::relaxed);
	const char* fileName = myReadFileName.load(std::memory_order::relaxed);
	uint_least32_t line = myReadLine.load(std::memory_order::relaxed);
	// most-significant bit is reserved for a write lock
	// check whether we just locked a write-locked mutex
	unsigned char currVal = myLock.fetch_or(kWriteVal);
	// if any of the bits are set, then a reader write/read-lock is active
	ASSERT_STR(currVal == 0, "Contention! {} at {}:{} tried to lock mutex for write "
		"while it's locked for read by {} at {}:{}",
		aLocation.function_name(), aLocation.file_name(), aLocation.line(),
		funcName, fileName, line
	);
}

void AssertRWMutex::UnlockWrite()
{
	myLock.fetch_xor(kWriteVal);
	{
		myWriteFuncName.store("", std::memory_order::relaxed);
		myWriteFileName.store("", std::memory_order::relaxed);
		myWriteLine.store(0, std::memory_order::relaxed);
		std::atomic_thread_fence(std::memory_order::release);
	}
}

void AssertRWMutex::LockRead(std::source_location aLocation)
{
	// we set these first  so that they are available in case of detected contention
	myReadFuncName.store(aLocation.function_name(), std::memory_order::relaxed);
	myReadFileName.store(aLocation.file_name(), std::memory_order::relaxed);
	myReadLine.store(aLocation.line(), std::memory_order::relaxed);
	std::atomic_thread_fence(std::memory_order::acq_rel);
	const char* funcName = myWriteFuncName.load(std::memory_order::relaxed);
	const char* fileName = myWriteFileName.load(std::memory_order::relaxed);
	uint_least32_t line = myWriteLine.load(std::memory_order::relaxed);
	// check whether we've just read-locked a write-locked mutex
	unsigned char currVal = myLock.fetch_add(1);
	ASSERT_STR((currVal & kWriteVal) == 0, "Contention! {} at {}:{} tried to lock mutex for read "
		"while it's locked for write by {} at {}:{}",
		aLocation.function_name(), aLocation.file_name(), aLocation.line(),
		funcName, fileName, line
	);
}

void AssertRWMutex::UnlockRead()
{
	uint8_t oldVal = myLock.fetch_sub(1);

	// We only reset the last location if there are no more reads
	if(oldVal == 1)
	{
		myReadFuncName.store("", std::memory_order::relaxed);
		myReadFileName.store("", std::memory_order::relaxed);
		myReadLine.store(0, std::memory_order::relaxed);
		std::atomic_thread_fence(std::memory_order::release);
	}
}

// ========================

AssertWriteLock::AssertWriteLock(AssertRWMutex& anRWMutex, std::source_location aLocation)
	: myMutex(anRWMutex)
{
	myMutex.LockWrite(aLocation);
}

AssertWriteLock::~AssertWriteLock()
{
	myMutex.UnlockWrite();
}

// ========================

AssertReadLock::AssertReadLock(AssertRWMutex& anRWMutex, std::source_location aLocation)
	: myMutex(anRWMutex)
{
	myMutex.LockRead(aLocation);
}

AssertReadLock::~AssertReadLock()
{
	myMutex.UnlockRead();
}

#endif // ASSERT_MUTEX