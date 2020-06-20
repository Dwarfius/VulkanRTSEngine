#include "Precomp.h"
#include "Serializer.h"

#include "Resource.h"
#include "AssetTracker.h"

Serializer::Serializer(AssetTracker& anAssetTracker, bool aIsReading)
	: myAssetTracker(anAssetTracker)
	, myIsReading(aIsReading)
{
}

void Serializer::SerializeVersion(int& aVersion)
{
	if (myIsReading)
	{
		// This covers cases where an pgrade has happened,
		// and we try to read a version, but there isn't one
		// which means we'll return version 1 by default
		aVersion = 1;
	}
	Serialize("version", aVersion);
}

template<class T, std::enable_if_t<Serializer::SupportedTypes::Contains<T>, bool> SFINAE>
void Serializer::Serialize(const std::string_view& aName, T& aValue)
{
	if (myIsReading)
	{
		VariantType variant = T{};
		DeserializeImpl(aName, variant);
		aValue = std::get<T>(variant);
	}
	else
	{
		const VariantType variant = aValue;
		SerializeImpl(aName, variant);
	}
}

template<class T>
void Serializer::Serialize(const std::string_view& aName, std::vector<T>& aValue)
{
	if (myIsReading)
	{
		VecVariantType vecVariant{ std::vector<T>{} };
		DeserializeImpl(aName, vecVariant, T{});
		aValue = std::move(std::get<std::vector<T>>(vecVariant));
	}
	else
	{
		// yeah it causes a copy, but ah well
		const VecVariantType vecVariant{ aValue };
		SerializeImpl(aName, vecVariant, T{});
	}
}

template void Serializer::Serialize(const std::string_view&, bool&);
template void Serializer::Serialize(const std::string_view&, uint32_t&);
template void Serializer::Serialize(const std::string_view&, int&);
template void Serializer::Serialize(const std::string_view&, float&);
template void Serializer::Serialize(const std::string_view&, std::string&);

template void Serializer::Serialize(const std::string_view&, std::vector<bool>&);
template void Serializer::Serialize(const std::string_view&, std::vector<uint32_t>&);
template void Serializer::Serialize(const std::string_view&, std::vector<int>&);
template void Serializer::Serialize(const std::string_view&, std::vector<float>&);
template void Serializer::Serialize(const std::string_view&, std::vector<std::string>&);