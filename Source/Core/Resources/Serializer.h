#pragma once

#include <variant>
#include "AssetTracker.h"

class File;
template<class T> class Handle;

class Serializer
{
	friend class AssetTracker;
	template<class... Types>
	struct VariantUtil
	{
		using VariantType = std::variant<Types...>;
		using VecVariantType = std::variant<std::vector<Types>...>;
	};
	
public:
	Serializer(AssetTracker& anAssetTracker, bool aIsReading);
	virtual ~Serializer() = default;

	template<class T>
	void Serialize(const std::string_view& aName, T& aValue);

	template<class T>
	void Serialize(const std::string_view& aName, std::vector<T>& aValue);

	template<class T>
	void Serialize(const std::string_view& aName, Handle<T>& aValue);

	template<class T>
	void Serialize(const std::string_view& aName, std::vector<Handle<T>>& aValue);

	void SerializeVersion(int& aVersion);

protected:
	struct ResourceProxy { std::string myPath; };
	using SupportedTypes = VariantUtil<bool, uint32_t, int, float, std::string, ResourceProxy>;
	using VariantType = SupportedTypes::VariantType;
	using VecVariantType = SupportedTypes::VecVariantType;
	bool myIsReading;

private:
	virtual void ReadFrom(const File& aFile) = 0;
	virtual void WriteTo(std::string& aBuffer) const = 0;
	virtual void SerializeImpl(const std::string_view& aName, const VariantType& aValue) = 0;
	virtual void SerializeImpl(const std::string_view& aName, const VecVariantType& aValue, const VariantType& aHint) = 0;
	virtual void DeserializeImpl(const std::string_view& aName, VariantType& aValue) = 0;
	virtual void DeserializeImpl(const std::string_view& aName, VecVariantType& aValue, const VariantType& aHint) = 0;

	AssetTracker& myAssetTracker;
};

template<class T>
void Serializer::Serialize(const std::string_view& aName, Handle<T>& aValue)
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
void Serializer::Serialize(const std::string_view& aName, std::vector<Handle<T>>& aValue)
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