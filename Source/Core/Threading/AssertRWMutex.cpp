#include "Precomp.h"
#include "AssertRWMutex.h"

#ifdef ASSERT_MUTEX

AssertRWMutex::AssertRWMutex()
	: myLock(0)
	, myLastReadThreadId(thread::id())
	, myWriteThreadId(thread::id())
{
}

void AssertRWMutex::LockWrite()
{
	// TODO: replace asserts with custom ones
	// most-significant bit is reserved for a write lock
	// check whether we just locked a write-locked mutex
	constexpr unsigned char writeVal = 1 << 7;
	unsigned char currVal = myLock.fetch_or(writeVal);
	if (currVal) // if any of the bits are set, then a reader write/read-lock is active
	{
		assert(false);
	}
	myWriteThreadId = this_thread::get_id();
}

void AssertRWMutex::UnlockWrite()
{
	// check whether we just unlocked an write-unlocked mutex
	constexpr unsigned char writeVal = 1 << 7;
	unsigned char currVal = myLock.fetch_xor(writeVal);
	if (currVal != writeVal)
	{
		assert(false);
	}
	myWriteThreadId = thread::id();
}

void AssertRWMutex::LockRead()
{
	// check whether we've just read-locked a write-locked mutex
	constexpr unsigned char writeVal = 1 << 7;
	unsigned char currVal = myLock.fetch_add(1);
	if (currVal & writeVal)
	{
		assert(false);
	}
	myLastReadThreadId = this_thread::get_id();
}

void AssertRWMutex::UnlockRead()
{
	// only check if were not unlocking an unlocked mutex
	unsigned char currVal = myLock.fetch_sub(1);
	if (currVal == 0)
	{
		assert(false);
	}
	// check whether we have just removed the last read lock
	if (currVal == 1)
	{
		// and if so, invalidate the thread index
		myLastReadThreadId = thread::id();
	}
}

// ========================

AssertWriteLock::AssertWriteLock(AssertRWMutex& anRWMutex)
	: myMutex(anRWMutex)
{
	myMutex.LockWrite();
}

AssertWriteLock::~AssertWriteLock()
{
	myMutex.UnlockWrite();
}

// ========================

AssertReadLock::AssertReadLock(AssertRWMutex& anRWMutex)
	: myMutex(anRWMutex)
{
	myMutex.LockRead();
}

AssertReadLock::~AssertReadLock()
{
	myMutex.UnlockRead();
}

#endif // ASSERT_MUTEX