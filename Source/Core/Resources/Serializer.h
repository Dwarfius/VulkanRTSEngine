#pragma once

#include "../VariantMap.h"
#include "AssetTracker.h"

class File;

class Serializer
{
	friend class AssetTracker;
	template<class... Types>
	struct VariantUtil
	{
		using VariantType = std::variant<Types...>;
		using VecVariantType = std::variant<std::vector<Types>...>;
		template<class T>
		constexpr static bool Contains = (std::is_same_v<T, Types> || ...);
	};

protected:
	struct ResourceProxy { std::string myPath; };
	using SupportedTypes = VariantUtil<bool, uint64_t, int64_t, float, std::string, VariantMap, ResourceProxy>;
	
public:
	Serializer(AssetTracker& anAssetTracker, bool aIsReading);
	virtual ~Serializer() = default;

	template<class T, std::enable_if_t<SupportedTypes::Contains<T>, bool> SFINAE = true>
	void Serialize(std::string_view aName, T& aValue);

	template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
	void Serialize(std::string_view aName, T& aValue)
	{
		// if reading, it'll update enumVal with correct value
		// if writing, it'll read from enumVal
		uint64_t enumVal = static_cast<uint32_t>(aValue);
		Serialize(aName, enumVal);
		aValue = static_cast<T>(enumVal);
	}

	template<class T>
	void Serialize(std::string_view aName, std::vector<T>& aValue);

	template<class T>
	void Serialize(std::string_view aName, Handle<T>& aValue);

	template<class T>
	void Serialize(std::string_view aName, std::vector<Handle<T>>& aValue);

	void SerializeVersion(int64_t& aVersion);

	bool IsReading() const { return myIsReading; }

protected:
	using VariantType = SupportedTypes::VariantType;
	using VecVariantType = SupportedTypes::VecVariantType;
	bool myIsReading;

private:
	virtual void ReadFrom(const File& aFile) = 0;
	virtual void WriteTo(std::string& aBuffer) const = 0;
	virtual void SerializeImpl(std::string_view aName, const VariantType& aValue) = 0;
	virtual void SerializeImpl(std::string_view aName, const VecVariantType& aValue, const VariantType& aHint) = 0;
	virtual void DeserializeImpl(std::string_view aName, VariantType& aValue) = 0;
	virtual void DeserializeImpl(std::string_view aName, VecVariantType& aValue, const VariantType& aHint) = 0;

	AssetTracker& myAssetTracker;
};

template<class T>
void Serializer::Serialize(std::string_view aName, Handle<T>& aValue)
{
	static_assert(std::is_base_of_v<Resource, T>, "Unsupported type!");
	if (myIsReading)
	{
		VariantType variant = ResourceProxy{};
		DeserializeImpl(aName, variant);
		const std::string& resPath = variant.get<ResourceProxy>().myPath;
		aValue = myAssetTracker.GetOrCreate<T>(resPath);
	}
	else
	{
		VariantType variant = ResourceProxy{ aValue->GetPath() };
		SerializeImpl(aName, variant);
	}
}

template<class T>
void Serializer::Serialize(std::string_view aName, std::vector<Handle<T>>& aValue)
{
	static_assert(std::is_base_of_v<Resource, T>, "Unsupported type!");
	if (myIsReading)
	{
		VecVariantType vecVariant{ std::vector<ResourceProxy>{} };
		DeserializeImpl(aName, vecVariant, ResourceProxy{});
		aValue.clear();
		const std::vector<ResourceProxy>& resourceVec = std::get<std::vector<ResourceProxy>>(vecVariant);
		aValue.resize(resourceVec.size());
		for (size_t i = 0; i < aValue.size(); i++)
		{
			const std::string& resPath = resourceVec[i].myPath;
			aValue[i] = std::move(myAssetTracker.GetOrCreate<T>(resPath));
		}
	}
	else
	{
		std::vector<ResourceProxy> resProxyVec;
		resProxyVec.resize(aValue.size());
		for (size_t i = 0; i < aValue.size(); i++)
		{
			resProxyVec[i] = ResourceProxy{ aValue[i]->GetPath() };
		}
		VecVariantType vecVariant = std::move(resProxyVec);
		SerializeImpl(aName, vecVariant, ResourceProxy{});
	}
}