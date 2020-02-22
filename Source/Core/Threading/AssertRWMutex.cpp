#include "Precomp.h"
#include "AssertRWMutex.h"

#ifdef ASSERT_MUTEX

#include <sstream>

AssertRWMutex::AssertRWMutex()
	: myLock(0)
	, myLastReadThreadId(std::thread::id())
	, myWriteThreadId(std::thread::id())
{
}

void AssertRWMutex::LockWrite()
{
	// most-significant bit is reserved for a write lock
	// check whether we just locked a write-locked mutex
	constexpr unsigned char writeVal = 1 << 7;
	unsigned char currVal = myLock.fetch_or(writeVal);
	if (currVal) // if any of the bits are set, then a reader write/read-lock is active
	{
		std::basic_stringstream<std::string::value_type> stream;
		stream << "Contention! Thread ";
		stream << std::this_thread::get_id();
		stream << " tried to lock mutex for write (W: ";
		stream << myWriteThreadId << ", R: ";
		stream << myLastReadThreadId << ")";
		ASSERT_STR(false, stream.str().c_str());
	}
	myWriteThreadId = std::this_thread::get_id();
}

void AssertRWMutex::UnlockWrite()
{
	// check whether we just unlocked an write-unlocked mutex
	// (double unlock)
	constexpr unsigned char writeVal = 1 << 7;
	unsigned char currVal = myLock.fetch_xor(writeVal);
	if (currVal != writeVal)
	{
		std::basic_stringstream<std::string::value_type> stream;
		stream << "Magic! Thread ";
		stream << std::this_thread::get_id();
		stream << " tried to unlock mutex for write (W: ";
		stream << myWriteThreadId << ", R: ";
		stream << myLastReadThreadId << ")";
		ASSERT_STR(false, stream.str().c_str());
	}
	myWriteThreadId = std::thread::id();
}

void AssertRWMutex::LockRead()
{
	// check whether we've just read-locked a write-locked mutex
	constexpr unsigned char writeVal = 1 << 7;
	unsigned char currVal = myLock.fetch_add(1);
	if (currVal & writeVal)
	{
		std::basic_stringstream<std::string::value_type> stream;
		stream << "Contention! Thread ";
		stream << std::this_thread::get_id();
		stream << " tried to lock mutex for read (W: ";
		stream << myWriteThreadId << ", R: ";
		stream << myLastReadThreadId << ")";
		ASSERT_STR(false, stream.str().c_str());
	}
	myLastReadThreadId = std::this_thread::get_id();
}

void AssertRWMutex::UnlockRead()
{
	// only check if were not unlocking an unlocked mutex
	unsigned char currVal = myLock.fetch_sub(1);
	if (currVal == 0)
	{
		std::basic_stringstream<std::string::value_type> stream;
		stream << "Magic! Thread ";
		stream << std::this_thread::get_id();
		stream << " tried to unlock mutex for read (W: ";
		stream << myWriteThreadId << ", R: ";
		stream << myLastReadThreadId << ")";
		ASSERT_STR(false, stream.str().c_str());
	}
	// check whether we have just removed the last read lock
	if (currVal == 1)
	{
		// and if so, invalidate the thread index
		myLastReadThreadId = std::thread::id();
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