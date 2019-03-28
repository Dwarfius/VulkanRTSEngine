#include "Precomp.h"
#include "Assert.h"

#ifdef ENABLE_ASSERTS
namespace DebugImpl
{
	enum class AssertAction
	{
		Continue,
		Break,
		Exit
	};

	// TODO: add more assert support
#	if defined(_WIN32)

	AssertAction ShowAssertDialog(const char* aTitle, const char* aMsg)
	{
		AssertAction action;
		int pickedOpt = MessageBox(0, aMsg, aTitle, MB_ABORTRETRYIGNORE);
		if (pickedOpt == IDABORT)
		{
			action = AssertAction::Exit;
		}
		else if (pickedOpt == IDRETRY)
		{
			action = AssertAction::Break;
		}
		else //if (pickedOpt == IDIGNORE)
		{
			// TODO: need to add proper ignore error support!
			action = AssertAction::Continue;
		}
		return action;
	}

	void TriggerBreakpoint()
	{
		__debugbreak();
	}

#	elif defined(__APPLE__)
static_assert(false, "Apple target not supported!");
#	elif defined(__linux__)
static_assert(false, "Linux target not supported!");
#	endif // _WIN32 / __APPLE__ / __linux__

	tbb::mutex locMutex;

	void AssertNotify(const char* anExpr, const char* aFile, int aLine, const char* aFmt, ...)
	{
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
			sprintf_s(fullMsg, "%s\nExpression %s has failed (file: %s:%d).", smallMsgBuffer, anExpr, aFile, aLine);
		}
		else
		{
			sprintf_s(fullMsg, "Expression %s has failed (file: %s:%d).", anExpr, aFile, aLine);
		}

		printf(fullMsg);
		printf("\n");

		// to avoid popping 20 message boxes, halt other threads until this one resolves
		locMutex.lock();
		AssertAction pickedAction = ShowAssertDialog("Assert Failed!", fullMsg);
		if (pickedAction == AssertAction::Break)
		{
			TriggerBreakpoint();
		}
		else if(pickedAction == AssertAction::Exit)
		{
			// before exiting, unlock the mutex so that it can get properly cleaned up
			locMutex.unlock();
			// using exit() instead of abort() to enable atexit() support
			exit(-1);
		}
		locMutex.unlock();
	}
}
#endif // ENABLE_ASSERTS