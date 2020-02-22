#include "Precomp.h"
#include "AssertMutex.h"

#ifdef ASSERT_MUTEX

#include <sstream>

AssertMutex::AssertMutex()
	: myWriter(std::thread::id())
{
}

void AssertMutex::Lock()
{
	std::thread::id expectedId = std::thread::id();
	std::thread::id currentThreadId = std::this_thread::get_id();
	// might need to change away from the default memory order to improve perf
	if (!myWriter.compare_exchange_strong(expectedId, currentThreadId))
	{
		std::string msg = "Contention! Thread ";
		std::basic_stringstream<std::string::value_type> stream(msg);
		stream << currentThreadId;
		stream << " tried to lock mutex owned by ";
		stream << expectedId;
		ASSERT_STR(false, msg.data());
	}
}

void AssertMutex::Unlock()
{
	std::thread::id expectedId = std::this_thread::get_id();
	std::thread::id invalidId = std::thread::id();
	if (!myWriter.compare_exchange_strong(expectedId, invalidId))
	{
		std::string msg = "Magic! Thread ";
		std::basic_stringstream<std::string::value_type> stream(msg);
		stream << std::this_thread::get_id();
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