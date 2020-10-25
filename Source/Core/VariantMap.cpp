#include "Precomp.h"
#include "VariantMap.h"

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Matches<T, std::is_convertible>, bool>>
void VariantMap::Get(const std::string& aKey, T& aValue) const
{
	using StorageType = SupportedTypes::FindMatchingType<T, std::is_convertible>;
	const Variant& variant = GetVariant(aKey);
	ASSERT_STR(std::holds_alternative<StorageType>(variant), "Invalid type!");
	aValue = std::get<StorageType>(variant);
}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Matches<T, std::is_convertible>, bool>>
void VariantMap::Get(size_t anIndex, T& aValue) const
{
  using StorageType = SupportedTypes::FindMatchingType<T, std::is_convertible>;
	Variant variant = GetVariant(anIndex);
	ASSERT_STR(std::holds_alternative<StorageType>(variant), "Invalid type!");
	aValue = std::get<StorageType>(variant);
}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Get(const std::string& aKey, std::vector<T>& aValue) const
{
	const Variant& variant = GetVariant(aKey);
	ASSERT_STR(std::holds_alternative<std::vector<T>>(variant), "Invalid type!");
	aValue = std::get<std::vector<T>>(variant);
}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Get(size_t anIndex, std::vector<T>& aValue) const
{
	Variant variant = GetVariant(anIndex);
	ASSERT_STR(std::holds_alternative<std::vector<T>>(variant), "Invalid type!");
	aValue = std::get<std::vector<T>>(variant);
}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Matches<T, std::is_convertible>, bool>>
void VariantMap::Set(const std::string& aKey, const T& aValue, bool aCanOverride /*= false*/)
{
	using StorageType = SupportedTypes::FindMatchingType<T, std::is_convertible>;
	ASSERT_STR(aCanOverride || myVariants.find(aKey) == myVariants.end(),
		"About to override a member!");
	myVariants.insert({ aKey, static_cast<StorageType>(aValue) });
}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Set(const std::string& aKey, const std::vector<T>& aValue, bool aCanOverride /*= false*/)
{
	ASSERT_STR(aCanOverride || myVariants.find(aKey) == myVariants.end(),
		"About to override a member!");
	myVariants.insert({ aKey, aValue });
}

const VariantMap::Variant& VariantMap::GetVariant(const std::string& aKey) const
{
	auto variantIter = myVariants.find(aKey);
	ASSERT_STR(variantIter != myVariants.end(), "Invalid key!");
	return variantIter->second;
}

const VariantMap::Variant& VariantMap::GetVariant(size_t anIndex) const
{
	ASSERT_STR(anIndex < myVariants.size(), "Out of bounds access!");
	auto variantIter = myVariants.begin();
	std::advance(myVariants.begin(), anIndex);
	return variantIter->second;
}

template void VariantMap::Get(const std::string&, bool&) const;
template void VariantMap::Get(const std::string&, uint64_t&) const;
template void VariantMap::Get(const std::string&, int64_t&) const;
template void VariantMap::Get(const std::string&, float&) const;
template void VariantMap::Get(const std::string&, std::string&) const;
template void VariantMap::Get(const std::string&, VariantMap&) const;

template void VariantMap::Get(size_t, bool&) const;
template void VariantMap::Get(size_t, uint64_t&) const;
template void VariantMap::Get(size_t, int64_t&) const;
template void VariantMap::Get(size_t, float&) const;
template void VariantMap::Get(size_t, std::string&) const;
template void VariantMap::Get(size_t, VariantMap&) const;

template void VariantMap::Get(const std::string&, std::vector<bool>&) const;
template void VariantMap::Get(const std::string&, std::vector<uint64_t>&) const;
template void VariantMap::Get(const std::string&, std::vector<int64_t>&) const;
template void VariantMap::Get(const std::string&, std::vector<float>&) const;
template void VariantMap::Get(const std::string&, std::vector<std::string>&) const;
template void VariantMap::Get(const std::string&, std::vector<VariantMap>&) const;

template void VariantMap::Get(size_t, std::vector<bool>&) const;
template void VariantMap::Get(size_t, std::vector<uint64_t>&) const;
template void VariantMap::Get(size_t, std::vector<int64_t>&) const;
template void VariantMap::Get(size_t, std::vector<float>&) const;
template void VariantMap::Get(size_t, std::vector<std::string>&) const;
template void VariantMap::Get(size_t, std::vector<VariantMap>&) const;

template void VariantMap::Set(const std::string&, const bool&, bool);
template void VariantMap::Set(const std::string&, const uint64_t&, bool);
template void VariantMap::Set(const std::string&, const int64_t&, bool);
template void VariantMap::Set(const std::string&, const float&, bool);
template void VariantMap::Set(const std::string&, const std::string&, bool);
template void VariantMap::Set(const std::string&, const VariantMap&, bool);
template void VariantMap::Set(const std::string&, const std::vector<bool>&, bool);

template void VariantMap::Set(const std::string&, const std::vector<uint64_t>&, bool);
template void VariantMap::Set(const std::string&, const std::vector<int64_t>&, bool);
template void VariantMap::Set(const std::string&, const std::vector<float>&, bool);
template void VariantMap::Set(const std::string&, const std::vector<std::string>&, bool);
template void VariantMap::Set(const std::string&, const std::vector<VariantMap>&, bool);

// integral conversion extensions
template void VariantMap::Get(const std::string&, uint8_t&) const;
template void VariantMap::Get(const std::string&, int8_t&) const;
template void VariantMap::Get(const std::string&, uint16_t&) const;
template void VariantMap::Get(const std::string&, int16_t&) const;
template void VariantMap::Get(const std::string&, uint32_t&) const;
template void VariantMap::Get(const std::string&, int32_t&) const;

template void VariantMap::Get(size_t, uint8_t&) const;
template void VariantMap::Get(size_t, int8_t&) const;
template void VariantMap::Get(size_t, uint16_t&) const;
template void VariantMap::Get(size_t, int16_t&) const;
template void VariantMap::Get(size_t, uint32_t&) const;
template void VariantMap::Get(size_t, int32_t&) const;

template void VariantMap::Set(const std::string&, const uint8_t&, bool);
template void VariantMap::Set(const std::string&, const int8_t&, bool);
template void VariantMap::Set(const std::string&, const uint16_t&, bool);
template void VariantMap::Set(const std::string&, const int16_t&, bool);
template void VariantMap::Set(const std::string&, const uint32_t&, bool);
template void VariantMap::Set(const std::string&, const int32_t&, bool);