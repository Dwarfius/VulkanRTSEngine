#pragma once

// Expanding macro, needed to help expand __VA_ARGS__ from "1, 2" to "1", "2"
#define DATA_ENUM_EXPAND(A) A

// Macro-Overloads
#define DATA_ENUM_FUNC0(Func, Arg, ...)
#define DATA_ENUM_FUNC1(Func, Arg, ...) Func(Arg)
#define DATA_ENUM_FUNC2(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC1(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC3(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC2(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC4(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC3(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC5(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC4(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC6(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC5(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC7(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC6(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC8(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC7(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC9(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC8(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC10(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC9(Func, __VA_ARGS__))

#define DATA_ENUM_PICK_11TH(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
// Determines the size of __VA_ARGS__ by sliding the correct responce to the
// "answer window" that DATA_ENUM_PICK_11TH provides
#define DATA_ENUM_NUM_ARGS(...) \
	DATA_ENUM_EXPAND(DATA_ENUM_PICK_11TH(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// Macro token pasting, needs 2 levels to process results of arguments rather
// their tokens
#define DATA_ENUM_COMBINE_(A, B) A ## B
#define DATA_ENUM_COMBINE(A, B) DATA_ENUM_COMBINE_(A, B)

// This macro first calculates how many variadic args there are
// Then stitches it with DATA_ENUM_FUNC to create a new macro token
// and calls it with passed in arguments
// Because we pass in __VA_ARGS__ to created overload macro, we need to
// force expansion with DATA_ENUM_EXPAND at top level
#define DATA_ENUM_FOR_EACH(Func, ...) \
	DATA_ENUM_EXPAND(DATA_ENUM_COMBINE(DATA_ENUM_FUNC, DATA_ENUM_NUM_ARGS(__VA_ARGS__))(Func, __VA_ARGS__))

#define DATA_ENUM_STRINGIFY(A) #A,

#define DATA_ENUM(Name, Type, ...) \
	struct Name \
	{ \
		constexpr static bool kIsEnum = true; \
		using UnderlyingType = Type; \
		\
		enum Values : UnderlyingType \
		{ \
			__VA_ARGS__ \
		}; \
		\
		constexpr static const char* const kNames[] \
		{ \
			DATA_ENUM_EXPAND(DATA_ENUM_FOR_EACH(DATA_ENUM_STRINGIFY, __VA_ARGS__)) \
		}; \
		\
		constexpr static size_t GetSize() \
		{ \
			return DATA_ENUM_EXPAND(DATA_ENUM_NUM_ARGS(__VA_ARGS__)); \
		} \
		\
		constexpr Name() = default; \
		constexpr Name(Values aValue) : myValue(aValue) {} \
        constexpr explicit Name(UnderlyingType aValue) : myValue(aValue) {} \
		constexpr explicit operator UnderlyingType() const { return myValue; } \
		\
		constexpr bool operator==(const Name& aOther) const { return myValue == aOther.myValue; } \
		constexpr bool operator!=(const Name& aOther) const { return myValue != aOther.myValue; } \
	private: \
		UnderlyingType myValue; \
	}