#pragma once

#ifndef ENABLE_ASSERTS
#	ifdef _DEBUG
#		define ENABLE_ASSERTS
#	endif // _DEBUG
#endif // ENABLE_ASSERTS

#ifdef assert
#	undef assert
#	define assert(x) static_assert(false, "Don't use lowercase assert, use ASSERT(cond)");
#endif

#if defined(ENABLE_ASSERTS)

// internal utility macro to force a semicolon after a macro in usercode
#define FORCE_SEMICOLON (void)0
// utility macro to execute code in assert-enabled environment
#define DEBUG_ONLY(x) x FORCE_SEMICOLON

// forward declaration of utility funcs
namespace DebugImpl
{
	void AssertNotify(const char* anExpr, const char* aFile, int aLine, const char* aFmt, ...);
};

// Basic assert
#define ASSERT(cond) if(!(cond)) { DebugImpl::AssertNotify(#cond, __FILE__, __LINE__, nullptr); } FORCE_SEMICOLON
// An assert that supports a formatted message. Use:
// either	ASSERT_STR(false, "Test 1");
// or		ASSERT_STR(false, "Test %d", 2);
#define ASSERT_STR(cond, ...) if(!(cond)) { DebugImpl::AssertNotify(#cond, __FILE__, __LINE__, __VA_ARGS__); } FORCE_SEMICOLON

#else // if !defined(ENABLE_ASSERTS)

#define ASSERT(cond)
#define ASSERT_STR(cond, ...)
#define DEBUG_ONLY(x)

#endif