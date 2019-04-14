#include "Precomp.h"
#include "Assert.h"

#ifdef ENABLE_ASSERTS
namespace DebugImpl
{
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
		else if (pickedOpt == IDIGNORE)
		{
			action = AssertAction::Ignore;
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
	AssertAction ShowAssertDialog(const char* aTitle, const char* aMsg)
	{
		return AssertAction::Break;
	}

#include <signal.h>

	void TriggerBreakpoint()
	{
		raise(SIGINT);
	}
#	endif // _WIN32 / __APPLE__ / __linux__
}
#endif // ENABLE_ASSERTS