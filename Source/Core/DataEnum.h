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
#define DATA_ENUM_FUNC11(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC10(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC12(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC11(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC13(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC12(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC14(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC13(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC15(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC14(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC16(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC15(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC17(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC16(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC18(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC17(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC19(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC18(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC20(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC19(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC21(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC20(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC22(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC21(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC23(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC22(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC24(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC23(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC25(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC24(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC26(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC25(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC27(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC26(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC28(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC27(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC29(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC28(Func, __VA_ARGS__))
#define DATA_ENUM_FUNC30(Func, Arg, ...) Func(Arg) DATA_ENUM_EXPAND(DATA_ENUM_FUNC29(Func, __VA_ARGS__))

#define DATA_ENUM_PICK_21TH(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
	_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, N, ...) N
// Determines the size of __VA_ARGS__ by sliding the correct responce to the
// "answer window" that DATA_ENUM_PICK_11TH provides
#define DATA_ENUM_NUM_ARGS(...) \
	DATA_ENUM_EXPAND(DATA_ENUM_PICK_21TH(__VA_ARGS__, \
	30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
	20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
	10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

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
		\
		constexpr operator Values() const { return static_cast<Values>(myValue); } \
		constexpr explicit operator UnderlyingType() const { return myValue; } \
	private: \
		UnderlyingType myValue; \
	}