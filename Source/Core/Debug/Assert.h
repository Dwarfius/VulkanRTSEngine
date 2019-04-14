#pragma once

#ifndef ENABLE_ASSERTS
#	if !defined(NDEBUG)
#		define ENABLE_ASSERTS
#	endif // _DEBUG
#endif // ENABLE_ASSERTS

#if defined(ENABLE_ASSERTS)

// internal concatenation that uses expanded macros
#define CONCAT(A, B) A B

// internal stringifier
#define STRINGIFY_IMPL(A) #A
// internal stringifier that expands macros
#define STRINGIFY(A) STRINGIFY_IMPL(A)

// internal utility macro to force a semicolon after a macro in usercode
#define FORCE_SEMICOLON (void)0
// utility macro to execute code in assert-enabled environment
#define DEBUG_ONLY(x) x FORCE_SEMICOLON

// forward declaration of utility funcs
namespace DebugImpl
{
	enum class AssertAction
	{
		Ignore,
		Break,
		Exit
	};

	AssertAction ShowAssertDialog(const char* aTitle, const char* aMsg);
	void TriggerBreakpoint();

	template<uint32_t HashVal>
	void AssertNotify(const char* anExpr, const char* aFile, int aLine, const char* aFmt, ...);
}

#include "../CRC32.h" // using Utils::CRC32

// Basic assert
#define ASSERT(cond) if(!(cond)) { DebugImpl::AssertNotify<Utils::CRC32(CONCAT(__FILE__, STRINGIFY(__LINE__)))>(#cond, __FILE__, __LINE__, nullptr); } FORCE_SEMICOLON
// An assert that supports a formatted message. Use:
// either	ASSERT_STR(false, "Test 1");
// or		ASSERT_STR(false, "Test %d", 2);
#define ASSERT_STR(cond, ...) if(!(cond)) { DebugImpl::AssertNotify<Utils::CRC32(CONCAT(__FILE__, STRINGIFY(__LINE__)))>(#cond, __FILE__, __LINE__, __VA_ARGS__); } FORCE_SEMICOLON

static_assert(Utils::CRC32("test", 4) == 0xD87F7E0C, "CRC32 Algo error!");

#include "Assert.inl"

#else // if !defined(ENABLE_ASSERTS)

#define ASSERT(cond)
#define ASSERT_STR(cond, ...)
#define DEBUG_ONLY(x)

#endif