#include <unordered_set>
#include <cstdarg>
#include <iostream>
#include <tbb/mutex.h>
#include "../Utils.h"

namespace DebugImpl
{
	static tbb::mutex assertMutex;
	static std::unordered_set<uint32_t> ignoreSet;

// Controls whether a single lock should control entire
// AssertNotify section, or should it be split into 2 parts -
// the hash check and AssertDialog
#define USE_DOUBLE_LOCK_IMPL

	template<uint32_t HashVal, class... TArgs>
	void AssertNotify(const char* anExpr, const char* aFile, int aLine, 
						const char* aFmt, const TArgs&... aArgs)
	{
		// first, check whether this assert should be ignored
		tbb::mutex::scoped_lock lock(assertMutex);
		if (ignoreSet.find(HashVal) != ignoreSet.end())
		{
			// found an ignored assert!
			return;
		}
#ifdef USE_DOUBLE_LOCK_IMPL
		lock.release();
#endif

		constexpr size_t cSmallMsgSize = 256;
		constexpr size_t cFullMsgSize = 1024;
		char smallMsgBuffer[cSmallMsgSize] = { 0 };
		if (aFmt)
		{
			Utils::StringFormat(smallMsgBuffer, aFmt, aArgs...);
		}
		char fullMsg[cFullMsgSize];
		if (aFmt)
		{
			Utils::StringFormat(fullMsg, "%s\nExpression %s has failed (file: %s:%d).\n", smallMsgBuffer, anExpr, aFile, aLine);
		}
		else
		{
			Utils::StringFormat(fullMsg, "Expression %s has failed (file: %s:%d).\n", anExpr, aFile, aLine);
		}

#ifdef USE_DOUBLE_LOCK_IMPL
		// to avoid popping 20 message boxes, halt other threads until this one resolves
		lock.acquire(assertMutex);
#endif
		std::cout << fullMsg;

		AssertAction pickedAction = ShowAssertDialog("Assert Failed!", fullMsg);
		if (pickedAction == AssertAction::Break)
		{
			TriggerBreakpoint();
		}
		else if(pickedAction == AssertAction::Exit)
		{
			// before exiting, unlock the mutex so that it can get properly cleaned up
			lock.release();
			// using exit() instead of abort() to enable atexit() support that
			// can be used by 3rd party libraries
			exit(-1);
		}
		else if (pickedAction == AssertAction::Ignore)
		{
			ignoreSet.insert(HashVal);
		}
	}
}