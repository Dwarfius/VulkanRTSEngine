#include <unordered_set>
#include <cstdarg>
#include <iostream>
#include "../Utils.h"

namespace DebugImpl
{
	static std::mutex assertMutex;
	static std::unordered_set<uint32_t> ignoreSet;

	template<uint32_t HashVal, class... TArgs>
	void AssertNotify(const char* anExpr, const char* aFile, int aLine, 
						const char* aFmt, const TArgs&... aArgs)
	{
		{
			// first, check whether this assert should be ignored
			std::lock_guard lock(assertMutex);
			if (ignoreSet.find(HashVal) != ignoreSet.end())
			{
				// found an ignored assert!
				return;
			}
		}

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

		{
			// to avoid popping 20 message boxes, halt other threads until this one resolves
			assertMutex.lock();
			std::cout << fullMsg;

			AssertAction pickedAction = ShowAssertDialog("Assert Failed!", fullMsg);
			if (pickedAction == AssertAction::Break)
			{
				TriggerBreakpoint();
			}
			else if (pickedAction == AssertAction::Exit)
			{
				// before exiting, unlock the mutex so that it can get properly cleaned up
				assertMutex.unlock();
				// using exit() instead of abort() to enable atexit() support that
				// can be used by 3rd party libraries
				exit(-1);
			}
			else if (pickedAction == AssertAction::Ignore)
			{
				ignoreSet.insert(HashVal);
			}
			assertMutex.unlock();
		}
	}
}