#pragma once

#include "StaticString.h"

namespace Utils
{
	template<class T>
	uint8_t CountSetBits(T aVal);

	bool IsNan(glm::vec3 aVec);
	bool IsNan(glm::vec4 aVec);

	// Groups copies of elements of TColl by TPred condition
	template<class TAssocColl, class TColl, class TPred>
	TAssocColl GroupBy(const TColl& aColl, TPred aPred);

	std::vector<char> Base64Encode(const std::vector<char>& anInput);
	std::vector<char> Base64Decode(const std::vector<char>& anInput);
	std::vector<char> Base64Decode(std::string_view anInput);

	template<uint32_t Length, class... TArgs>
	void StringFormat(char(& aBuffer)[Length], const char* aFormat, const TArgs&... aArgs);

	// tries to match the filter to the string. Supports * wildcard
	bool Matches(std::string_view aStr, std::string_view filter);

	constexpr size_t Align(size_t aValue, size_t anAlignment)
	{
		const size_t remainder = aValue % anAlignment;
		return remainder ? aValue + (anAlignment - remainder) : aValue;
	}

	template<class T>
	consteval auto NameOfFunc()
	{
#if defined(_MSC_VER)
		constexpr std::string_view funcName = __FUNCSIG__;
		constexpr std::string_view startTag = "NameOfFunc<";
		constexpr char endTag = '>';
#elif defined(__clang__)
		constexpr std::string_view funcName = __PRETTY_FUNCTION__;
		constexpr std::string_view startTag = "[T = ";
		constexpr char endTag = ']';
#elif defined(__GNUC__)
		constexpr std::string_view funcName = __PRETTY_FUNCTION__;
		constexpr std::string_view startTag = "[with T = ";
		constexpr char endTag = ';';
#else
#error Unsupported Compiler
#endif
		constexpr size_t startInd = funcName.find(startTag) + startTag.size();
		constexpr size_t endInd = funcName.rfind(endTag);
		constexpr std::string_view name = funcName.substr(startInd, endInd - startInd);
		// it might be scope-qualified or namespace qualified
		// need to strip all of that
#if defined(_MSC_VER)
		constexpr size_t qualInd = name.rfind("::");
		constexpr std::string_view noQualName = qualInd != std::string_view::npos ? name.substr(qualInd + 2) : name;
		// MSVC also can specify "struct/class T", so need to
		// strip it out
		constexpr size_t typeInd = noQualName.rfind(' ');
		constexpr std::string_view res = typeInd != std::string_view::npos ? noQualName.substr(typeInd + 1) : noQualName;
#elif defined(__GNUC__)
		constexpr size_t qualInd = name.rfind("::");
		constexpr std::string_view res = qualInd != std::string_view::npos ? name.substr(qualInd + 2) : name;
#else
		constexpr std::string_view res = name;
#endif

		char buffer[res.size() + 1]{ 0 };
		res.copy(buffer, res.size());
		return StaticString(buffer);
	}

	template<class T>
	constexpr static StaticString NameOf = NameOfFunc<T>();
}
static_assert(Utils::NameOf<int> == "int");

template<class T>
uint8_t Utils::CountSetBits(T aVal)
{
	static_assert(std::is_integral_v<T>, "T mush be integral!");
	uint8_t bitCount = 0;
	while (aVal)
	{
		bitCount += aVal & 1;
		aVal >>= 1;
	}
	return bitCount;
}

template<class TAssocColl, class TColl, class TPred>
TAssocColl Utils::GroupBy(const TColl& aColl, TPred aPred)
{
	TAssocColl assocCollection;
	for (const auto& item : aColl)
	{
		auto key = aPred(item);
		assocCollection[key].push_back(item);
	}
	return assocCollection;
}

template<uint32_t Length, class... TArgs>
void Utils::StringFormat(char(&aBuffer)[Length], const char* aFormat, const TArgs&... aArgs)
{
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_WANT_SECURE_LIB__)
	sprintf_s(aBuffer, aFormat, aArgs...);
#else
	std::snprintf(aBuffer, Length, aFormat, aArgs...);
#endif
}

namespace Utils
{
	class AffinitySetter : public tbb::task_scheduler_observer
	{
	public:
		enum class Priority
		{
			High,
			Medium,
			Low
		};

		AffinitySetter(tbb::task_arena& anArena, Priority aPriority)
			: tbb::task_scheduler_observer(anArena)
			, myPriority(aPriority)
		{
			observe();
		}

		AffinitySetter(Priority aPriority)
			: myPriority(aPriority)
		{
			observe();
		}

	private:
		void on_scheduler_entry(bool aWorker) override;
		void on_scheduler_exit(bool aWorker) override;

		const Priority myPriority;
	};
}