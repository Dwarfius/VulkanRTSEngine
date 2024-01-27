#include "Precomp.h"
#include "BinarySerializer.h"

namespace Impl
{
	// Serialization of (unsigned-)integrals by octets
	template<class T>
	void SerializeBasic(std::vector<char>& aBuffer, size_t& anIndex, T& aValue, bool aIsReading)
	{
		using UnsignedT = std::make_unsigned_t<T>;
		constexpr size_t kIters = sizeof(UnsignedT) / sizeof(char);
		
		if (aIsReading)
		{
			UnsignedT unsignedValue = 0;
			for (uint8_t i = 0; i < kIters; i++)
			{
				uint8_t byteVal = static_cast<uint8_t>(aBuffer[anIndex + i]);
				unsignedValue |= byteVal << (i * 8);
			}
			aValue = std::bit_cast<T>(unsignedValue);
			anIndex += kIters;
		}
		else
		{
			size_t currPos = aBuffer.size();
			aBuffer.resize(aBuffer.size() + kIters);

			UnsignedT unsignedValue = std::bit_cast<UnsignedT>(aValue);
			for (uint8_t i = 0; i < kIters; i++)
			{
				uint8_t byteVal = static_cast<uint8_t>(unsignedValue >> (i * 8));
				aBuffer[currPos + i] = byteVal;
			}
		}
	}

	template<>
	void SerializeBasic<bool>(std::vector<char>& aBuffer, size_t& anIndex, bool& aValue, bool aIsReading)
	{
		uint8_t value = std::bit_cast<uint8_t>(aValue);
		SerializeBasic(aBuffer, anIndex, value, aIsReading);
		aValue = std::bit_cast<bool>(value);
	}

	template<>
	void SerializeBasic<float>(std::vector<char>& aBuffer, size_t& anIndex, float& aValue, bool aIsReading)
	{
		uint32_t value = std::bit_cast<uint32_t>(aValue);
		SerializeBasic(aBuffer, anIndex, value, aIsReading);
		aValue = std::bit_cast<float>(value);
	}

	template<class T>
	void SerializeSpan(std::vector<char>& aBuffer, size_t& anIndex, T* aValues, size_t aSize, bool aIsReading)
	{
		using UnsignedT = std::make_unsigned_t<T>;
		constexpr size_t kIters = sizeof(UnsignedT) / sizeof(char);

		if (aIsReading)
		{
			for (size_t elem = 0; elem < aSize; elem++)
			{
				UnsignedT unsignedValue = 0;
				for (uint8_t i = 0; i < kIters; i++)
				{
					uint8_t byteVal = static_cast<uint8_t>(aBuffer[anIndex + i]);
					unsignedValue |= byteVal << (i * 8);
				}
				aValues[elem] = std::bit_cast<T>(unsignedValue);
				anIndex += kIters;
			}
		}
		else
		{
			size_t currPos = aBuffer.size();
			aBuffer.resize(aBuffer.size() + kIters * aSize);

			for (size_t elem = 0; elem < aSize; elem++)
			{
				UnsignedT unsignedValue = std::bit_cast<UnsignedT>(aValues[elem]);
				for (uint8_t i = 0; i < kIters; i++)
				{
					uint8_t byteVal = static_cast<uint8_t>(unsignedValue >> (i * 8));
					aBuffer[currPos + elem * kIters + i] = byteVal;
				}
			}
		}
	}

	template<>
	void SerializeSpan<bool>(std::vector<char>& aBuffer, size_t& anIndex, bool* aValues, size_t aSize, bool aIsReading)
	{
		if (aIsReading)
		{
			for (size_t elem = 0; elem < aSize; elem++)
			{
				aValues[elem] = aBuffer[anIndex + elem] != 0;
			}
		}
		else
		{
			size_t currPos = aBuffer.size();
			aBuffer.resize(aBuffer.size() + aSize);

			for (size_t elem = 0; elem < aSize; elem++)
			{
				aBuffer[currPos + elem] = static_cast<uint8_t>(aValues[elem]);
			}
		}
	}

	template<>
	void SerializeSpan<float>(std::vector<char>& aBuffer, size_t& anIndex, float* aValues, size_t aSize, bool aIsReading)
	{
		using UnsignedT = uint32_t;
		constexpr size_t kIters = sizeof(float) / sizeof(char);

		if (aIsReading)
		{
			for (size_t elem = 0; elem < aSize; elem++)
			{
				UnsignedT unsignedValue = 0;
				for (uint8_t i = 0; i < kIters; i++)
				{
					uint8_t byteVal = static_cast<uint8_t>(aBuffer[anIndex + i]);
					unsignedValue |= byteVal << (i * 8);
				}
				aValues[elem] = std::bit_cast<float>(unsignedValue);
				anIndex += kIters;
			}
		}
		else
		{
			size_t currPos = aBuffer.size();
			aBuffer.resize(aBuffer.size() + kIters * aSize);

			for (size_t elem = 0; elem < aSize; elem++)
			{
				UnsignedT unsignedValue = std::bit_cast<UnsignedT>(aValues[elem]);
				for (uint8_t i = 0; i < kIters; i++)
				{
					uint8_t byteVal = static_cast<uint8_t>(unsignedValue >> (i * 8));
					aBuffer[currPos + elem * kIters + i] = byteVal;
				}
			}
		}
	}
}

void BinarySerializer::ReadFrom(const std::vector<char>& aBuffer)
{
	myBuffer = aBuffer;
	myIndex = 0;
}

void BinarySerializer::WriteTo(std::vector<char>& aBuffer) const
{
	aBuffer = myBuffer;
}

void BinarySerializer::Serialize(std::string_view, bool& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, uint8_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, uint16_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, uint32_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, uint64_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, int8_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, int16_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, int32_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, int64_t& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, float& aValue)
{
	Impl::SerializeBasic(myBuffer, myIndex, aValue, IsReading());
}

void BinarySerializer::Serialize(std::string_view, std::string& aValue)
{
	const bool isReading = IsReading();
	size_t size = aValue.size();
	Impl::SerializeBasic(myBuffer, myIndex, size, isReading);
	aValue.resize(size);

	Impl::SerializeSpan(myBuffer, myIndex, aValue.data(), size, isReading);
}

void BinarySerializer::Serialize(std::string_view, glm::vec2& aValue)
{
	Impl::SerializeSpan(myBuffer, myIndex, glm::value_ptr(aValue), 2, IsReading());
}

void BinarySerializer::Serialize(std::string_view, glm::vec3& aValue)
{
	Impl::SerializeSpan(myBuffer, myIndex, glm::value_ptr(aValue), 3, IsReading());
}

void BinarySerializer::Serialize(std::string_view, glm::vec4& aValue)
{
	Impl::SerializeSpan(myBuffer, myIndex, glm::value_ptr(aValue), 4, IsReading());
}

void BinarySerializer::Serialize(std::string_view, glm::quat& aValue)
{
	Impl::SerializeSpan(myBuffer, myIndex, glm::value_ptr(aValue), 4, IsReading());
}

void BinarySerializer::Serialize(std::string_view, glm::mat4& aValue)
{
	Impl::SerializeSpan(myBuffer, myIndex, glm::value_ptr(aValue), 16, IsReading());
}

void BinarySerializer::SerializeExternal(std::string_view aFile, std::vector<char>& aBlob, Resource::Id)
{
	std::string filename(aFile);
	Serialize("myExternFile", filename);
	size_t size = aBlob.size();
	Serialize("myExternFileSize", size);
	aBlob.resize(size);

	Impl::SerializeSpan(myBuffer, myIndex, aBlob.data(), size, IsReading());
}

bool BinarySerializer::BeginSerializeObjectImpl(std::string_view)
{
	// NOOP
	return true;
}

void BinarySerializer::EndSerializeObjectImpl(std::string_view)
{
	// NOOP
}

bool BinarySerializer::BeginSerializeArrayImpl(std::string_view aName, size_t& aCount)
{
	Impl::SerializeBasic(myBuffer, myIndex, aCount, IsReading());
	return true;
}

void BinarySerializer::EndSerializeArrayImpl(std::string_view)
{
	// NOOP
}

void BinarySerializer::SerializeSpan(bool* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(uint8_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(uint16_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(uint32_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(uint64_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(int8_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(int16_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(int32_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(int64_t* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(float* aValues, size_t aSize) 
{
	Impl::SerializeSpan(myBuffer, myIndex, aValues, aSize, IsReading());
}

void BinarySerializer::SerializeSpan(std::string* aValues, size_t aSize) 
{
	const bool isReading = IsReading();
	for (size_t i = 0; i < aSize; i++)
	{
		std::string& value = aValues[i];
		size_t size = value.size();
		Impl::SerializeBasic(myBuffer, myIndex, size, isReading);
		value.resize(size);

		Impl::SerializeSpan(myBuffer, myIndex, value.data(), size, isReading);
	}
}

void BinarySerializer::SerializeSpan(glm::vec2* aValues, size_t aSize) 
{
	const bool isReading = IsReading();
	for (size_t i = 0; i < aSize; i++)
	{
		Impl::SerializeSpan(myBuffer, myIndex, 
			glm::value_ptr(aValues[i]), 2, isReading);
	}
}

void BinarySerializer::SerializeSpan(glm::vec3* aValues, size_t aSize)
{
	const bool isReading = IsReading();
	for (size_t i = 0; i < aSize; i++)
	{
		Impl::SerializeSpan(myBuffer, myIndex,
			glm::value_ptr(aValues[i]), 3, isReading);
	}
}

void BinarySerializer::SerializeSpan(glm::vec4* aValues, size_t aSize) 
{
	const bool isReading = IsReading();
	for (size_t i = 0; i < aSize; i++)
	{
		Impl::SerializeSpan(myBuffer, myIndex,
			glm::value_ptr(aValues[i]), 4, isReading);
	}
}

void BinarySerializer::SerializeSpan(glm::quat* aValues, size_t aSize) 
{
	const bool isReading = IsReading();
	for (size_t i = 0; i < aSize; i++)
	{
		Impl::SerializeSpan(myBuffer, myIndex,
			glm::value_ptr(aValues[i]), 4, isReading);
	}
}

void BinarySerializer::SerializeSpan(glm::mat4* aValues, size_t aSize) 
{
	const bool isReading = IsReading();
	for (size_t i = 0; i < aSize; i++)
	{
		Impl::SerializeSpan(myBuffer, myIndex,
			glm::value_ptr(aValues[i]), 16, isReading);
	}
}

void BinarySerializer::SerializeEnum(std::string_view aName, size_t& anEnumValue, const char* const* aNames, size_t aNamesLength)
{
	Impl::SerializeBasic(myBuffer, myIndex, anEnumValue, IsReading());
}