#include "Precomp.h"
#include "AssertMutex.h"

#ifdef ASSERT_MUTEX

#include <sstream>

void AssertMutex::Lock(std::source_location aLocation)
{
	std::atomic_thread_fence(std::memory_order::acquire);
	const char* funcName = myFuncName.load(std::memory_order::relaxed);
	const char* fileName = myFileName.load(std::memory_order::relaxed);
	uint_least32_t line = myLine.load(std::memory_order::relaxed);

	myFuncName.store(aLocation.function_name(), std::memory_order::relaxed);
	myFileName.store(aLocation.file_name(), std::memory_order::relaxed);
	myLine.store(aLocation.line(), std::memory_order::relaxed);
	std::atomic_thread_fence(std::memory_order::release);
	if (myIsLocked.test_and_set())
	{
		ASSERT_STR(false, "Contention! {} at {}:{} tried to lock mutex owned by {} at {}:{}",
			aLocation.function_name(), aLocation.file_name(), aLocation.line(),
			funcName, fileName, line
		);
	}
}

void AssertMutex::Unlock()
{
	myIsLocked.clear();
	myFuncName.store("", std::memory_order::relaxed);
	myFileName.store("", std::memory_order::relaxed);
	myLine.store(0, std::memory_order::relaxed);
	std::atomic_thread_fence(std::memory_order::release);
}

// ========================

AssertLock::AssertLock(AssertMutex& aMutex, std::source_location aLocation)
	: myMutex(aMutex)
{
	myMutex.Lock(aLocation);
}

AssertLock::~AssertLock()
{
	myMutex.Unlock();
}

#endif // ASSERT_MUTEX