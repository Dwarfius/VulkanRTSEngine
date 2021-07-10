#include "Precomp.h"
#include "Serializer.h"

#include "Resource.h"
#include "AssetTracker.h"

Serializer::Scope::Scope(Serializer& aSerializer, std::string_view aName, Serializer::State aNewState, bool aIsValid)
	: mySerializer(aSerializer)
	, myNameIndexVariant(aName)
	, myState(aNewState)
	, myPrevState(State::Object)
	, myIsValid(aIsValid)
{
}

Serializer::Scope::Scope(Serializer& aSerializer, size_t anIndex, Serializer::State aNewState, bool aIsValid)
	: mySerializer(aSerializer)
	, myNameIndexVariant(anIndex)
	, myState(aNewState)
	, myPrevState(State::Array)
	, myIsValid(aIsValid)
{
}

Serializer::Scope::~Scope()
{
	// unroll state
	mySerializer.myState = myPrevState;

	if (!myIsValid) // TODO: [[unlikely]]
	{
		// it's an invalid scope, meaning Serializer failed to find Object or Array
		// meaning there's nothing to close down
		return;
	}

	if (mySerializer.IsReading())
	{
		switch (myState)
		{
		case State::Object: 
			if (myPrevState == State::Object)
			{
				mySerializer.EndDeserializeObjectImpl(std::get<std::string_view>(myNameIndexVariant));
			}
			else
			{
				mySerializer.EndDeserializeObjectImpl(std::get<size_t>(myNameIndexVariant));
			}
			break;
		case State::Array: 
			if (myPrevState == State::Object)
			{
				mySerializer.EndDeserializeArrayImpl(std::get<std::string_view>(myNameIndexVariant));
			}
			else
			{
				mySerializer.EndDeserializeArrayImpl(std::get<size_t>(myNameIndexVariant));
			}
			break;
		default: ASSERT(false);
		}
	}
	else
	{
		switch (myState)
		{
		case State::Object: 
			if (myPrevState == State::Object)
			{
				mySerializer.EndSerializeObjectImpl(std::get<std::string_view>(myNameIndexVariant));
			}
			else
			{
				mySerializer.EndSerializeObjectImpl(std::get<size_t>(myNameIndexVariant));
			} 
			break;
		case State::Array: 
			if (myPrevState == State::Object)
			{
				mySerializer.EndSerializeArrayImpl(std::get<std::string_view>(myNameIndexVariant));
			}
			else
			{
				mySerializer.EndSerializeArrayImpl(std::get<size_t>(myNameIndexVariant));
			} 
			break;
		default: ASSERT(false);
		}
	}
}

Serializer::Serializer(AssetTracker& anAssetTracker, bool aIsReading)
	: myAssetTracker(anAssetTracker)
	, myIsReading(aIsReading)
{
}

template<class T, std::enable_if_t<Serializer::SupportedTypes::Contains<T> 
	|| Serializer::SupportedTypes::Matches<T, IsImplicitlyPromotable>, bool> SFINAE>
void Serializer::Serialize(std::string_view aName, T& aValue)
{
	ASSERT_STR(myState == State::Object, "Invalid call, currently working with an object!");
	
	using StorageType = std::conditional_t<Serializer::SupportedTypes::Contains<T>, 
		T, 
		SupportedTypes::FindMatchingType<T, IsImplicitlyPromotable>
	>;
	if (myIsReading)
	{
		VariantType variant = StorageType{ aValue };
		DeserializeImpl(aName, variant);
		aValue = static_cast<T>(std::get<StorageType>(variant));
	}
	else
	{
		const VariantType variant = static_cast<StorageType>(aValue);
		SerializeImpl(aName, variant);
	}
}

template<class T, std::enable_if_t<Serializer::SupportedTypes::Contains<T>
	|| Serializer::SupportedTypes::Matches<T, IsImplicitlyPromotable>, bool> SFINAE>
void Serializer::Serialize(size_t anIndex, T& aValue)
{
	ASSERT_STR(myState == State::Array, "Invalid call, currently working with an array!");

	using StorageType = std::conditional_t<Serializer::SupportedTypes::Contains<T>,
		T,
		SupportedTypes::FindMatchingType<T, IsImplicitlyPromotable>
	>;
	if (myIsReading)
	{
		VariantType variant = StorageType{ aValue };
		DeserializeImpl(anIndex, variant);
		aValue = static_cast<T>(std::get<StorageType>(variant));
	}
	else
	{
		const VariantType variant = static_cast<StorageType>(aValue);
		SerializeImpl(anIndex, variant);
	}
}

Serializer::Scope Serializer::SerializeObject(std::string_view aName)
{
	ASSERT_STR(myState == State::Object, "Invalid call, currently working with an object!");

	bool isValid = true;
	if (myIsReading)
	{
		isValid = BeginDeserializeObjectImpl(aName);
	}
	else
	{
		BeginSerializeObjectImpl(aName);
	}
	myState = State::Object;
	return Scope(*this, aName, myState, isValid);
}

Serializer::Scope Serializer::SerializeObject(size_t anIndex)
{
	ASSERT_STR(myState == State::Array, "Invalid call, currently working with an array!");

	bool isValid = true;
	if (myIsReading)
	{
		isValid = BeginDeserializeObjectImpl(anIndex);
	}
	else
	{
		BeginSerializeObjectImpl(anIndex);
	}
	myState = State::Object;
	return Scope(*this, anIndex, myState, isValid);
}

Serializer::Scope Serializer::SerializeArray(std::string_view aName, size_t& anArraySize)
{
	ASSERT_STR(myState == State::Object, "Invalid call, currently working with an object!");

	bool isValid = true;
	if (myIsReading)
	{
		isValid = BeginDeserializeArrayImpl(aName, anArraySize);
	}
	else
	{
		BeginSerializeArrayImpl(aName, anArraySize);
	}
	myState = State::Array;
	return Scope(*this, aName, myState, isValid);
}

Serializer::Scope Serializer::SerializeArray(size_t anIndex, size_t& anArraySize)
{
	ASSERT_STR(myState == State::Array, "Invalid call, currently working with an array!");

	bool isValid = true;
	if (myIsReading)
	{
		isValid = BeginDeserializeArrayImpl(anIndex, anArraySize);
	}
	else
	{
		BeginSerializeArrayImpl(anIndex, anArraySize);
	}
	myState = State::Array;
	return Scope(*this, anIndex, myState, isValid);
}

void Serializer::SerializeVersion(int64_t& aVersion)
{
	if (myIsReading)
	{
		// This covers cases where an upgrade has happened,
		// and we try to read a version, but there isn't one
		// which means we'll return version 1 by default
		aVersion = 1;
	}
	Serialize("version", aVersion);
}

template void Serializer::Serialize(std::string_view, bool&);
template void Serializer::Serialize(size_t, bool&);
template void Serializer::Serialize(std::string_view, uint64_t&);
template void Serializer::Serialize(size_t, uint64_t&);
template void Serializer::Serialize(std::string_view, int64_t&);
template void Serializer::Serialize(size_t, int64_t&);
template void Serializer::Serialize(std::string_view, float&);
template void Serializer::Serialize(size_t, float&);
template void Serializer::Serialize(std::string_view, std::string&);
template void Serializer::Serialize(size_t, std::string&);

template void Serializer::Serialize(std::string_view, glm::vec2&);
template void Serializer::Serialize(size_t, glm::vec2&);
template void Serializer::Serialize(std::string_view, glm::vec3&);
template void Serializer::Serialize(size_t, glm::vec3&);
template void Serializer::Serialize(std::string_view, glm::vec4&);
template void Serializer::Serialize(size_t, glm::vec4&);
template void Serializer::Serialize(std::string_view, glm::quat&);
template void Serializer::Serialize(size_t, glm::quat&);
template void Serializer::Serialize(std::string_view, glm::mat4&);
template void Serializer::Serialize(size_t, glm::mat4&);

// integral conversion extensions
template void Serializer::Serialize(std::string_view, uint8_t&);
template void Serializer::Serialize(size_t, uint8_t&);
template void Serializer::Serialize(std::string_view, int8_t&);
template void Serializer::Serialize(size_t, int8_t&);
template void Serializer::Serialize(std::string_view, uint16_t&);
template void Serializer::Serialize(size_t, uint16_t&);
template void Serializer::Serialize(std::string_view, int16_t&);
template void Serializer::Serialize(size_t, int16_t&);
template void Serializer::Serialize(std::string_view, uint32_t&);
template void Serializer::Serialize(size_t, uint32_t&);
template void Serializer::Serialize(std::string_view, int32_t&);
template void Serializer::Serialize(size_t, int32_t&);