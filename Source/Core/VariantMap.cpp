#include "Precomp.h"
#include "VariantMap.h"

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Get(const std::string& aKey, T& aValue) const
{
	const Variant& variant = GetVariant(aKey);
	ASSERT_STR(std::holds_alternative<T>(variant), "Invalid type!");
	aValue = std::get<T>(variant);
}

//template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
//void VariantMap::Get(size_t anIndex, T& aValue) const
//{
//	Variant variant = GetVariant(anIndex);
//	ASSERT_STR(std::holds_alternative<T>(variant), "Invalid type!");
//	aValue = std::get<T>(variant);
//}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Get(const std::string& aKey, std::vector<T>& aValue) const
{
	const Variant& variant = GetVariant(aKey);
	ASSERT_STR(std::holds_alternative<std::vector<T>>(variant), "Invalid type!");
	aValue = std::get<std::vector<T>>(variant);
}

//template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
//void VariantMap::Get(size_t anIndex, std::vector<T>& aValue) const
//{
//	Variant variant = GetVariant(anIndex);
//	ASSERT_STR(std::holds_alternative<std::vector<T>>(variant), "Invalid type!");
//	aValue = std::get<std::vector<T>>(variant);
//}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Set(const std::string& aKey, const T& aValue, bool aCanOverride /*= false*/)
{
	ASSERT_STR(aCanOverride || myVariants.find(aKey) == myVariants.end(),
		"About to override a member!");
	myVariants.insert({ aKey, aValue });
}

template<class T, std::enable_if_t<VariantMap::SupportedTypes::Contains<T>, bool>>
void VariantMap::Set(const std::string& aKey, const std::vector<T>& aValue, bool aCanOverride /*= false*/)
{
	ASSERT_STR(aCanOverride || myVariants.find(aKey) == myVariants.end(),
		"About to override a member!");
	myVariants.insert({ aKey, Variant { aValue } });
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

//template void VariantMap::Get(size_t, bool&) const;
//template void VariantMap::Get(size_t, uint64_t&) const;
//template void VariantMap::Get(size_t, int64_t&) const;
//template void VariantMap::Get(size_t, float&) const;
//template void VariantMap::Get(size_t, std::string&) const;
//template void VariantMap::Get(size_t, VariantMap&) const;

template void VariantMap::Get(const std::string&, std::vector<bool>&) const;
template void VariantMap::Get(const std::string&, std::vector<uint64_t>&) const;
template void VariantMap::Get(const std::string&, std::vector<int64_t>&) const;
template void VariantMap::Get(const std::string&, std::vector<float>&) const;
template void VariantMap::Get(const std::string&, std::vector<std::string>&) const;
template void VariantMap::Get(const std::string&, std::vector<VariantMap>&) const;

//template void VariantMap::Get(size_t, std::vector<bool>&) const;
//template void VariantMap::Get(size_t, std::vector<uint64_t>&) const;
//template void VariantMap::Get(size_t, std::vector<int64_t>&) const;
//template void VariantMap::Get(size_t, std::vector<float>&) const;
//template void VariantMap::Get(size_t, std::vector<std::string>&) const;
//template void VariantMap::Get(size_t, std::vector<VariantMap>&) const;

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

template<>
void VariantMap::Get<uint8_t>(const std::string& aKey, uint8_t& aValue) const
{
    uint64_t value = 0;
    Get(aKey, value);
    aValue = static_cast<uint8_t>(value);
}

template<>
void VariantMap::Get<int8_t>(const std::string& aKey, int8_t& aValue) const
{
    int64_t value = 0;
    Get(aKey, value);
    aValue = static_cast<int8_t>(value);
}

template<>
void VariantMap::Get<uint16_t>(const std::string& aKey, uint16_t& aValue) const
{
    uint64_t value = 0;
    Get(aKey, value);
    aValue = static_cast<uint16_t>(value);
}

template<>
void VariantMap::Get<int16_t>(const std::string& aKey, int16_t& aValue) const
{
    int64_t value = 0;
    Get(aKey, value);
    aValue = static_cast<int16_t>(value);
}

template<>
void VariantMap::Get<uint32_t>(const std::string& aKey, uint32_t& aValue) const
{
    uint64_t value = 0;
    Get(aKey, value);
    aValue = static_cast<uint32_t>(value);
}

template<>
void VariantMap::Get<int32_t>(const std::string& aKey, int32_t& aValue) const
{
    int64_t value = 0;
    Get(aKey, value);
    aValue = static_cast<int32_t>(value);
}

template<>
void VariantMap::Set<uint8_t>(const std::string& aKey, const uint8_t& aValue, bool aCanOverride /*= false*/)
{
    Set(aKey, static_cast<uint8_t>(aValue), aCanOverride);
}

template<>
void VariantMap::Set<int8_t>(const std::string& aKey, const int8_t& aValue, bool aCanOverride /*= false*/)
{
    Set(aKey, static_cast<int8_t>(aValue), aCanOverride);
}

template<>
void VariantMap::Set<uint16_t>(const std::string& aKey, const uint16_t& aValue, bool aCanOverride /*= false*/)
{
    Set(aKey, static_cast<uint16_t>(aValue), aCanOverride);
}

template<>
void VariantMap::Set<int16_t>(const std::string& aKey, const int16_t& aValue, bool aCanOverride /*= false*/)
{
    Set(aKey, static_cast<int16_t>(aValue), aCanOverride);
}

template<>
void VariantMap::Set<uint32_t>(const std::string& aKey, const uint32_t& aValue, bool aCanOverride /*= false*/)
{
    Set(aKey, static_cast<uint32_t>(aValue), aCanOverride);
}

template<>
void VariantMap::Set<int32_t>(const std::string& aKey, const int32_t& aValue, bool aCanOverride /*= false*/)
{
    Set(aKey, static_cast<int32_t>(aValue), aCanOverride);
}