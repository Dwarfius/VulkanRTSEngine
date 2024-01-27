#pragma once

#include "AssetTracker.h"
#include "../StaticVector.h"

class Serializer;

namespace SerializerConcepts
{
	template<class T>
	concept DataEnum = T::kIsEnum;

	template<class T>
	concept Serializable = requires(T t, Serializer& aSerializer)
	{
		{ t.Serialize(aSerializer) };
	};

	template<class T>
	concept Resource = std::is_base_of_v<::Resource, T>;
}

class Serializer
{
public:
	// Denotes that the element being serialized with this name is part of
	// an array (so it doesn't have a name, but some index)
	constexpr static std::string_view kArrayElem = "";

	class ObjectScope
	{
	public:
		ObjectScope(Serializer& aSerializer, std::string_view aName)
			: mySerializer(aSerializer)
			, myName(aName)
		{
			myIsOpen = mySerializer.BeginSerializeObjectImpl(myName);
		}

		~ObjectScope()
		{
			if (myIsOpen)
			{
				mySerializer.EndSerializeObjectImpl(myName);
			}
		}

		operator bool() const
		{
			return myIsOpen;
		}

	private:
		Serializer& mySerializer;
		std::string_view myName;
		bool myIsOpen;
	};

	// Note: be warned, arrays-of-arrays are not supported!
	class ArrayScope
	{
	public:
		ArrayScope(Serializer& aSerializer, std::string_view aName, size_t& aSize)
			: mySerializer(aSerializer)
			, myName(aName)
		{
			myIsOpen = mySerializer.BeginSerializeArrayImpl(myName, aSize);
		}

		~ArrayScope()
		{
			if (myIsOpen)
			{
				mySerializer.EndSerializeArrayImpl(myName);
			}
		}

		operator bool() const
		{
			return myIsOpen;
		}

	private:
		Serializer& mySerializer;
		std::string_view myName;
		bool myIsOpen;
	};

public:
	Serializer(AssetTracker& anAssetTracker, bool aIsReading);
	virtual ~Serializer() = default;

	virtual void ReadFrom(const std::vector<char>& aBuffer) = 0;
	virtual void WriteTo(std::vector<char>& aBuffer) const = 0;

	virtual void Serialize(std::string_view aName, bool& aValue) = 0;
	virtual void Serialize(std::string_view aName, uint8_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, uint16_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, uint32_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, uint64_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, int8_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, int16_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, int32_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, int64_t& aValue) = 0;
	virtual void Serialize(std::string_view aName, float& aValue) = 0;
	virtual void Serialize(std::string_view aName, std::string& aValue) = 0;
	virtual void Serialize(std::string_view aName, glm::vec2& aValue) = 0;
	virtual void Serialize(std::string_view aName, glm::vec3& aValue) = 0;
	virtual void Serialize(std::string_view aName, glm::vec4& aValue) = 0;
	virtual void Serialize(std::string_view aName, glm::quat& aValue) = 0;
	virtual void Serialize(std::string_view aName, glm::mat4& aValue) = 0;

	template<SerializerConcepts::DataEnum T>
	void Serialize(std::string_view aName, T& aValue);

	template<SerializerConcepts::Serializable T>
	void Serialize(std::string_view aName, T& anObject);

	template<SerializerConcepts::Resource T>
	void Serialize(std::string_view aName, Handle<T>& anObject);

	template<class T>
	void Serialize(std::string_view aName, std::vector<T>& aVec);

	template<class T, uint16_t N>
	void Serialize(std::string_view aName, StaticVector<T, N>& aVec);

	template<SerializerConcepts::DataEnum T>
	void Serialize(std::string_view aName, std::vector<T>& aVec);

	template<SerializerConcepts::DataEnum T, uint16_t N>
	void Serialize(std::string_view aName, StaticVector<T, N>& aVec);

	template<SerializerConcepts::Serializable T>
	void Serialize(std::string_view aName, std::vector<T>& aVec);

	template<SerializerConcepts::Serializable T, uint16_t N>
	void Serialize(std::string_view aName, StaticVector<T, N>& aVec);

	template<SerializerConcepts::Resource T>
	void Serialize(std::string_view aName, std::vector<Handle<T>>& aVec);

	template<SerializerConcepts::Resource T, uint16_t N>
	void Serialize(std::string_view aName, StaticVector<T, N>& aVec);

	void SerializeVersion(uint8_t& aVersion);
	virtual void SerializeExternal(std::string_view aFile, std::vector<char>& aBlob, Resource::Id anId) = 0;

	bool IsReading() const { return myIsReading; }

	AssetTracker& GetAssetTracker() { return myAssetTracker; }

protected:
	struct ResourceProxy
	{
		std::string myPath;

		void Serialize(Serializer& aSerializer)
		{
			aSerializer.Serialize("myPath", myPath);
		}
	};

private:
	// Note: aName == "" treated that it's part of an array!
	[[nodiscard]]
	virtual bool BeginSerializeObjectImpl(std::string_view aName) = 0;
	// Note: aName == "" treated that it's part of an array!
	virtual void EndSerializeObjectImpl(std::string_view aName) = 0;

	// Note: aName == "" treated that it's part of an array!
	[[nodiscard]]
	virtual bool BeginSerializeArrayImpl(std::string_view aName, size_t& aCount) = 0;
	// Note: aName == "" treated that it's part of an array!
	virtual void EndSerializeArrayImpl(std::string_view aName) = 0;

	virtual void SerializeResource(std::string_view aName, ResourceProxy& aValue);
	virtual void SerializeEnum(std::string_view aName, size_t& anEnumValue, const char* const* aNames, size_t aNamesLength) = 0;

	virtual void SerializeSpan(bool* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(uint8_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(uint16_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(uint32_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(uint64_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(int8_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(int16_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(int32_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(int64_t* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(float* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(std::string* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(glm::vec2* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(glm::vec3* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(glm::vec4* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(glm::quat* aValues, size_t aSize) = 0;
	virtual void SerializeSpan(glm::mat4* aValues, size_t aSize) = 0;

	AssetTracker& myAssetTracker;
	bool myIsReading;
};

template<SerializerConcepts::DataEnum T>
void Serializer::Serialize(std::string_view aName, T& aValue)
{
	size_t enumValue = static_cast<T::UnderlyingType>(aValue);
	SerializeEnum(aName, enumValue, T::kNames, T::GetSize());
	aValue = T(static_cast<T::UnderlyingType>(enumValue));
}

template<SerializerConcepts::Serializable T>
void Serializer::Serialize(std::string_view aName, T& anObject)
{
	if (BeginSerializeObjectImpl(aName))
	{
		anObject.Serialize(*this);
		EndSerializeObjectImpl(aName);
	}
}

template<SerializerConcepts::Resource T>
void Serializer::Serialize(std::string_view aName, Handle<T>& anObject)
{
	ResourceProxy resProxy;
	if (anObject.IsValid())
	{
		resProxy.myPath = anObject->GetPath();
	}
	SerializeResource(aName, resProxy);

	if (myIsReading && !resProxy.myPath.empty())
	{
		anObject = myAssetTracker.GetOrCreate<T>(resProxy.myPath);
	}
}

template<class T>
void Serializer::Serialize(std::string_view aName, std::vector<T>& aVec)
{
	size_t vecSize = aVec.size();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		aVec.resize(vecSize);
		SerializeSpan(aVec.data(), aVec.size());
		EndSerializeArrayImpl(aName);
	}
}

template<class T, uint16_t N>
void Serializer::Serialize(std::string_view aName, StaticVector<T, N>& aVec)
{
	size_t vecSize = aVec.GetSize();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		vecSize = glm::min<size_t>(vecSize, N);
		aVec.Resize(vecSize);
		SerializeSpan(aVec.data(), vecSize);
		EndSerializeArrayImpl(aName);
	}
}

template<SerializerConcepts::DataEnum T>
void Serializer::Serialize(std::string_view aName, std::vector<T>& aVec)
{
	size_t vecSize = aVec.size();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		aVec.resize(vecSize);
		for(T& enumValue : aVec)
		{
			SerializeEnum(kArrayElem, enumValue);
		}
		EndSerializeArrayImpl(aName);
	}
}

template<SerializerConcepts::DataEnum T, uint16_t N>
void Serializer::Serialize(std::string_view aName, StaticVector<T, N>& aVec)
{
	size_t vecSize = aVec.GetSize();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		vecSize = glm::min<size_t>(vecSize, N);
		aVec.Resize(vecSize);
		for (size_t i = 0; i < vecSize; i++)
		{
			T& enumValue = *(aVec.data() + i);
			SerializeEnum(kArrayElem, enumValue);
		}
		EndSerializeArrayImpl(aName);
	}
}

template<SerializerConcepts::Serializable T>
void Serializer::Serialize(std::string_view aName, std::vector<T>& aVec)
{
	size_t vecSize = aVec.size();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		aVec.resize(vecSize);
		for (T& object : aVec)
		{
			Serialize("", object);
		}
		EndSerializeArrayImpl(aName);
	}
}

template<SerializerConcepts::Serializable T, uint16_t N>
void Serializer::Serialize(std::string_view aName, StaticVector<T, N>& aVec)
{
	size_t vecSize = aVec.GetSize();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		vecSize = glm::min<size_t>(vecSize, N);
		aVec.Resize(vecSize);
		for (size_t i = 0; i < vecSize; i++)
		{
			T& object = *(aVec.data() + i);
			Serialize(kArrayElem, object);
		}
		EndSerializeArrayImpl(aName);
	}
}

template<SerializerConcepts::Resource T>
void Serializer::Serialize(std::string_view aName, std::vector<Handle<T>>& aVec)
{
	size_t vecSize = aVec.size();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		aVec.resize(vecSize);
		for (Handle<T>& handle : aVec)
		{
			Serialize(kArrayElem, handle);
		}
		EndSerializeArrayImpl(aName);
	}
}

template<SerializerConcepts::Resource T, uint16_t N>
void Serializer::Serialize(std::string_view aName, StaticVector<T, N>& aVec)
{
	size_t vecSize = aVec.GetSize();
	if (BeginSerializeArrayImpl(aName, vecSize))
	{
		vecSize = glm::min<size_t>(vecSize, N);
		aVec.Resize(vecSize);
		for (size_t i = 0; i < vecSize; i++)
		{
			Handle<T>& object = *(aVec.data() + i);
			Serialize(kArrayElem, object);
		}
		EndSerializeArrayImpl(aName);
	}
}