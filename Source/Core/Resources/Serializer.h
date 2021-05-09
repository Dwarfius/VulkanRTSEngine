#pragma once

#include "../VariantMap.h"
#include "AssetTracker.h"
#include "../VariantUtils.h"

class Serializer
{
	enum class State : uint8_t
	{
		Object, // serializing an object (collection of variables)
		Array // serializing an array
	};

protected:
	struct ResourceProxy { std::string myPath; };
	using SupportedTypes = VariantUtil<bool, uint64_t, int64_t, float, 
		std::string, VariantMap, ResourceProxy,
		glm::vec2, glm::vec3, glm::vec4, glm::quat, glm::mat4
	>;
	using VariantType = SupportedTypes::VariantType;
	
public:
	// Utility struct to help unrol Serializer state
	// and close down object/array serialization on destruction
	class Scope
	{
		friend class Serializer;
		Scope(Serializer& aSerializer, std::string_view aName, Serializer::State aNewState, bool aIsValid);
		Scope(Serializer& aSerializer, size_t anIndex, Serializer::State aNewState, bool aIsValid);

		Scope(const Scope&) = delete;

		Serializer& mySerializer;
		std::variant<std::string_view, size_t> myNameIndexVariant;
		Serializer::State myState;
		Serializer::State myPrevState;
		bool myIsValid;
	public:
		~Scope();
		operator bool() const { return myIsValid; }
	};

	Serializer(AssetTracker& anAssetTracker, bool aIsReading);
	virtual ~Serializer() = default;

	template<class T, std::enable_if_t<SupportedTypes::Contains<T> || SupportedTypes::Matches<T, IsImplicitlyPromotable>, bool> SFINAE = true>
	void Serialize(std::string_view aName, T& aValue);

	template<class T, std::enable_if_t<SupportedTypes::Contains<T> || SupportedTypes::Matches<T, IsImplicitlyPromotable>, bool> SFINAE = true>
	void Serialize(size_t anIndex, T& aValue);

	template<class T>
	void Serialize(std::string_view aName, Handle<T>& aValue);

	template<class T>
	void Serialize(size_t anIndex, Handle<T>& aValue);

	template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
	void Serialize(std::string_view aName, T& aValue)
	{
		ASSERT_STR(myState == State::Object, "Invalid call, currently working with an object!");
		// if reading, it'll update enumVal with correct value
		// if writing, it'll read from enumVal
		uint64_t enumVal = static_cast<uint64_t>(aValue);
		Serialize(aName, enumVal);
		aValue = static_cast<T>(enumVal);
	}

	template<class T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
	void Serialize(size_t anIndex, T& aValue)
	{
		ASSERT_STR(myState == State::Array, "Invalid call, currently working with an array!");
		// if reading, it'll update enumVal with correct value
		// if writing, it'll read from enumVal
		uint64_t enumVal = static_cast<uint64_t>(aValue);
		Serialize(anIndex, enumVal);
		aValue = static_cast<T>(enumVal);
	}

	Scope SerializeObject(std::string_view aName);
	Scope SerializeObject(size_t anIndex);

	template<class T>
	Scope SerializeArray(std::string_view aName, std::vector<T>& aVec);
	template<class T>
	Scope SerializeArray(size_t anIndex, std::vector<T>& aVec);

	Scope SerializeArray(std::string_view aName, size_t& anArraySize);
	Scope SerializeArray(size_t anIndex, size_t& anArraySize);

	void SerializeVersion(int64_t& aVersion);
	virtual void SerializeExternal(std::string_view aFile, std::vector<char>& aBlob) = 0;

	bool IsReading() const { return myIsReading; }

	virtual void ReadFrom(const std::vector<char>& aBuffer) = 0;
	virtual void WriteTo(std::vector<char>& aBuffer) const = 0;
	
private:
	virtual void SerializeImpl(std::string_view aName, const VariantType& aValue) = 0;
	virtual void SerializeImpl(size_t anIndex, const VariantType& aValue) = 0;
	virtual void DeserializeImpl(std::string_view aName, VariantType& aValue) const = 0;
	virtual void DeserializeImpl(size_t anIndex, VariantType& aValue) const = 0;

	virtual void BeginSerializeObjectImpl(std::string_view aName) = 0;
	virtual void BeginSerializeObjectImpl(size_t anIndex) = 0;
	virtual void EndSerializeObjectImpl(std::string_view aName) = 0;
	virtual void EndSerializeObjectImpl(size_t anIndex) = 0;
	// TODO: [[nodiscard]]
	virtual bool BeginDeserializeObjectImpl(std::string_view aName) const = 0;
	// TODO: [[nodiscard]]
	virtual bool BeginDeserializeObjectImpl(size_t anIndex) const = 0;
	virtual void EndDeserializeObjectImpl(std::string_view aName) const = 0;
	virtual void EndDeserializeObjectImpl(size_t anIndex) const = 0;

	virtual void BeginSerializeArrayImpl(std::string_view aName, size_t aCount) = 0;
	virtual void BeginSerializeArrayImpl(size_t anIndex, size_t aCount) = 0;
	virtual void EndSerializeArrayImpl(std::string_view aName) = 0;
	virtual void EndSerializeArrayImpl(size_t anIndex) = 0;
	// TODO: [[nodiscard]]
	virtual bool BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const = 0;
	// TODO: [[nodiscard]]
	virtual bool BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const = 0;
	virtual void EndDeserializeArrayImpl(std::string_view aName) const = 0;
	virtual void EndDeserializeArrayImpl(size_t anIndex) const = 0;

	AssetTracker& myAssetTracker;
	State myState = State::Object;
	bool myIsReading;
};

template<class T>
void Serializer::Serialize(std::string_view aName, Handle<T>& aValue)
{
	static_assert(std::is_base_of_v<Resource, T>, "T must inherit from Resource!");

	ASSERT_STR(myState == State::Object, "Invalid call, currently working with an object!");
	if (myIsReading)
	{
		VariantType variant = ResourceProxy{};
		DeserializeImpl(aName, variant);
		const std::string& resPath = std::get<ResourceProxy>(variant).myPath;
		aValue = myAssetTracker.GetOrCreate<T>(resPath);
	}
	else
	{
		VariantType variant = ResourceProxy{ aValue->GetPath() };
		SerializeImpl(aName, variant);
	}
}

template<class T>
void Serializer::Serialize(size_t anIndex, Handle<T>& aValue)
{
	static_assert(std::is_base_of_v<Resource, T>, "T must inherit from Resource!");
	
	ASSERT_STR(myState == State::Array, "Invalid call, currently working with an array!");
	if (myIsReading)
	{
		VariantType variant = ResourceProxy{};
		DeserializeImpl(anIndex, variant);
		const std::string& resPath = std::get<ResourceProxy>(variant).myPath;
		aValue = myAssetTracker.GetOrCreate<T>(resPath);
	}
	else
	{
		VariantType variant = ResourceProxy{ aValue->GetPath() };
		SerializeImpl(anIndex, variant);
	}
}

template<class T>
Serializer::Scope Serializer::SerializeArray(std::string_view aName, std::vector<T>& aVec)
{
	ASSERT_STR(myState == State::Object, "Invalid call, currently working with an object!");

	bool isValid = true;
	if (myIsReading)
	{
		size_t vecSize = aVec.size();
		isValid = BeginDeserializeArrayImpl(aName, vecSize);
		aVec.resize(vecSize);
	}
	else
	{
		BeginSerializeArrayImpl(aName, aVec.size());
	}
	myState = State::Array;
	return Scope(*this, aName, myState, isValid);
}

template<class T>
Serializer::Scope Serializer::SerializeArray(size_t anIndex, std::vector<T>& aVec)
{
	ASSERT_STR(myState == State::Array, "Invalid call, currently working with an array!");

	bool isValid = true;
	if (myIsReading)
	{
		size_t vecSize = aVec.size();
		isValid = BeginDeserializeArrayImpl(anIndex, vecSize);
		aVec.resize(vecSize);
	}
	else
	{
		BeginSerializeArrayImpl(anIndex, aVec.size());
	}
	myState = State::Array;
	return Scope(*this, anIndex, myState, isValid);
}