#pragma once

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
}

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