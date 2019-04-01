#include <unordered_set>

namespace DebugImpl
{
	static tbb::mutex assertMutex;
	static std::unordered_set<uint32_t> ignoreSet;

// Controls whether a single lock should control entire
// AssertNotify section, or should it be split into 2 parts -
// the hash check and AssertDialog
#define USE_DOUBLE_LOCK_IMPL

	// TODO: add compile time hashing using crc32c 
	// (https://stackoverflow.com/questions/10953958/can-crc32-be-used-as-a-hash-function)
	template<uint32_t HashVal>
	void AssertNotify(const char* anExpr, const char* aFile, int aLine, const char* aFmt, ...)
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
		char smallMsgBuffer[cSmallMsgSize + 1] = { 0 };
		if (aFmt)
		{
			va_list args;
			va_start(args, aFmt);
			vsnprintf_s(smallMsgBuffer, cSmallMsgSize, aFmt, args);
			va_end(args);
		}
		char fullMsg[cFullMsgSize + 1];
		if (aFmt)
		{
			sprintf_s(fullMsg, "%s\nExpression %s has failed (file: %s:%d).\n", smallMsgBuffer, anExpr, aFile, aLine);
		}
		else
		{
			sprintf_s(fullMsg, "Expression %s has failed (file: %s:%d).\n", anExpr, aFile, aLine);
		}

#ifdef USE_DOUBLE_LOCK_IMPL
		// to avoid popping 20 message boxes, halt other threads until this one resolves
		lock.acquire(assertMutex);
#endif
		printf(fullMsg);

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
			// be used by 3rd party libraries
			exit(-1);
		}
		else if (pickedAction == AssertAction::Continue)
		{
			ignoreSet.insert(HashVal);
		}
	}
}