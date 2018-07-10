#include "Precomp.h"
#include "AssertMutex.h"

#ifdef ASSERT_MUTEX

#include <sstream>

AssertMutex::AssertMutex()
	: myWriter(thread::id())
{
}

void AssertMutex::Lock()
{
	thread::id expectedId = thread::id();
	thread::id currentThreadId = this_thread::get_id();
	// might need to change away from the default memory order to improve perf
	if (!myWriter.compare_exchange_strong(expectedId, currentThreadId))
	{
		string msg = "Contention! Thread ";
		basic_stringstream<string::value_type> stream(msg);
		stream << currentThreadId;
		stream << " tried to lock mutex owned by ";
		stream << expectedId;
		ASSERT_STR(false, msg.data());
	}
}

void AssertMutex::Unlock()
{
	thread::id expectedId = this_thread::get_id();
	thread::id invalidId = thread::id();
	if (!myWriter.compare_exchange_strong(expectedId, invalidId))
	{
		string msg = "Magic! Thread ";
		basic_stringstream<string::value_type> stream(msg);
		stream << this_thread::get_id();
		stream << " tried to unlock mutex owned by ";
		stream << expectedId;
		ASSERT_STR(false, msg.data());
	}
}

// ========================

AssertLock::AssertLock(AssertMutex& aMutex)
	: myMutex(aMutex)
{
	myMutex.Lock();
}

AssertLock::~AssertLock()
{
	myMutex.Unlock();
}

#endif // ASSERT_MUTEX