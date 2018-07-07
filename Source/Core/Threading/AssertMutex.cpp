#include "Precomp.h"
#include "AssertMutex.h"

#ifdef ASSERT_MUTEX

AssertMutex::AssertMutex()
	: myWriter(thread::id())
{
}

void AssertMutex::Lock()
{
	thread::id expectedId = thread::id();
	// might need to change away from the default memory order to improve perf
	if (!myWriter.compare_exchange_strong(expectedId, this_thread::get_id()))
	{
		// TODO: replace with new assert
		assert(false);
	}
}

void AssertMutex::Unlock()
{
	thread::id expectedId = this_thread::get_id();
	thread::id invalidId = thread::id();
	if (!myWriter.compare_exchange_strong(expectedId, invalidId))
	{
		assert(false);
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