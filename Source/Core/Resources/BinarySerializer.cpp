#include "Precomp.h"
#include "BinarySerializer.h"

void BinarySerializer::ReadFrom(const std::vector<char>& aBuffer)
{
	myBuffer = aBuffer;
	myIndex = 0;
}

void BinarySerializer::WriteTo(std::vector<char>& aBuffer) const
{
	aBuffer = myBuffer;
}

void BinarySerializer::SerializeImpl(std::string_view, const VariantType& aValue)
{
	std::visit([&](auto&& aVarValue) {
		Write(aVarValue);
	}, aValue);
}

void BinarySerializer::SerializeImpl(size_t, const VariantType& aValue)
{
	std::visit([&](auto&& aVarValue) {
		Write(aVarValue);
	}, aValue);
}

void BinarySerializer::DeserializeImpl(std::string_view, VariantType& aValue) const
{
	std::visit([&](auto&& aVarValue) {
		using Type = std::decay_t<decltype(aVarValue)>;
		Type valueToRead;
		Read(valueToRead);
		aVarValue = valueToRead;
	}, aValue);
}

void BinarySerializer::DeserializeImpl(size_t, VariantType& aValue) const
{
	std::visit([&](auto&& aVarValue) {
		using Type = std::decay_t<decltype(aVarValue)>;
		Type valueToRead;
		Read(valueToRead);
		aVarValue = valueToRead;
	}, aValue);
}

void BinarySerializer::BeginSerializeObjectImpl(std::string_view)
{
	// NOOP
}

void BinarySerializer::BeginSerializeObjectImpl(size_t)
{
	// NOOP
}

void BinarySerializer::EndSerializeObjectImpl(std::string_view)
{
	// NOOP
}

void BinarySerializer::EndSerializeObjectImpl(size_t)
{
	// NOOP
}

bool BinarySerializer::BeginDeserializeObjectImpl(std::string_view) const
{
	// NOOP
	return true;
}

bool BinarySerializer::BeginDeserializeObjectImpl(size_t) const
{
	// NOOP
	return true;
}

void BinarySerializer::EndDeserializeObjectImpl(std::string_view) const
{
	// NOOP
}

void BinarySerializer::EndDeserializeObjectImpl(size_t) const
{
	// NOOP
}

void BinarySerializer::BeginSerializeArrayImpl(std::string_view aName, size_t aCount)
{
	SerializeImpl(aName, aCount);
}

void BinarySerializer::BeginSerializeArrayImpl(size_t anIndex, size_t aCount)
{
	SerializeImpl(anIndex, aCount);
}

void BinarySerializer::EndSerializeArrayImpl(std::string_view)
{
	// NOOP
}

void BinarySerializer::EndSerializeArrayImpl(size_t)
{
	// NOOP
}

bool BinarySerializer::BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const
{
	VariantType variant = size_t{};
	DeserializeImpl(aName, variant);
	aCount = std::get<size_t>(variant);
	return true;
}

bool BinarySerializer::BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const
{
	VariantType variant = size_t{};
	DeserializeImpl(anIndex, variant);
	aCount = std::get<size_t>(variant);
	return true;
}

void BinarySerializer::EndDeserializeArrayImpl(std::string_view) const
{
	// NOOP
}

void BinarySerializer::EndDeserializeArrayImpl(size_t) const
{
	// NOOP
}

void BinarySerializer::Write(bool aValue)
{
	myBuffer.push_back(aValue);
}

void BinarySerializer::Read(bool& aValue) const
{
	aValue = myBuffer[myIndex] != 0;
	myIndex++;
}

void BinarySerializer::Write(uint64_t aValue)
{
	constexpr size_t kIters = sizeof(aValue) / sizeof(char);
	size_t currPos = myBuffer.size();
	myBuffer.resize(myBuffer.size() + kIters);
	for (size_t i = 0; i < kIters; i++)
	{
		const char byteVal = static_cast<const char>(aValue >> (i * 8));
		myBuffer[currPos + i] = byteVal;
	}
}

void BinarySerializer::Read(uint64_t& aValue) const
{
	constexpr size_t kIters = sizeof(aValue) / sizeof(char);
	aValue = 0;
	for (size_t i = 0; i < kIters; i++)
	{
		unsigned char byteVal = myBuffer[myIndex + i];
		uint64_t value = byteVal;
		aValue |= value << (i * 8);
	}
	myIndex += kIters;
}

void BinarySerializer::Write(int64_t aValue)
{
	// TODO: C++20 std::bitcast
	uint64_t value;
	std::memcpy(&value, &aValue, sizeof(aValue));
	Write(value);
}

void BinarySerializer::Read(int64_t& aValue) const
{
	uint64_t value;
	Read(value);
	// TODO: C++20 std::bitcast
	std::memcpy(&aValue, &value, sizeof(value));
}

void BinarySerializer::Write(float aValue)
{
	// TODO: C++20 std::bitcast
	uint64_t value;
	std::memcpy(&value, &aValue, sizeof(aValue));
	Write(value);
}

void BinarySerializer::Read(float& aValue) const
{
	uint64_t value;
	Read(value);
	// TODO: C++20 std::bitcast
	std::memcpy(&aValue, &value, sizeof(value));
}

void BinarySerializer::Write(const std::string& aValue)
{
	// Size
	Write(aValue.size());

	// Contents
	const size_t kIters = aValue.size();
	size_t currPos = myBuffer.size();
	myBuffer.resize(myBuffer.size() + kIters);
	for (size_t i = 0; i < kIters; i++)
	{
		myBuffer[currPos + i] = aValue[i];
	}
}

void BinarySerializer::Read(std::string& aValue) const
{
	// Size
	uint64_t size;
	Read(size);

	// Contents
	aValue.resize(size);
	for (size_t i = 0; i < size; i++)
	{
		aValue[i] = myBuffer[myIndex + i];
	}
	myIndex += size;
}

void BinarySerializer::Write(const VariantMap& aValue)
{
	ASSERT_STR(false, "NYI");
}

void BinarySerializer::Read(VariantMap& aValue) const
{
	ASSERT_STR(false, "NYI");
}

void BinarySerializer::Write(const ResourceProxy& aValue)
{
	Write(aValue.myPath);
}

void BinarySerializer::Read(ResourceProxy& aValue) const
{
	Read(aValue.myPath);
}

void BinarySerializer::Write(const glm::vec2& aValue)
{
	Write(aValue.x);
	Write(aValue.y);
}

void BinarySerializer::Read(glm::vec2& aValue) const
{
	Read(aValue.x);
	Read(aValue.y);
}

void BinarySerializer::Write(const glm::vec3& aValue)
{
	Write(aValue.x);
	Write(aValue.y);
	Write(aValue.z);
}

void BinarySerializer::Read(glm::vec3& aValue) const
{
	Read(aValue.x);
	Read(aValue.y);
	Read(aValue.z);
}

void BinarySerializer::Write(const glm::vec4& aValue)
{
	Write(aValue.x);
	Write(aValue.y);
	Write(aValue.z);
	Write(aValue.w);
}

void BinarySerializer::Read(glm::vec4& aValue) const
{
	Read(aValue.x);
	Read(aValue.y);
	Read(aValue.z);
	Read(aValue.w);
}

void BinarySerializer::Write(const glm::quat& aValue)
{
	Write(aValue.x);
	Write(aValue.y);
	Write(aValue.z);
	Write(aValue.w);
}

void BinarySerializer::Read(glm::quat& aValue) const
{
	Read(aValue.x);
	Read(aValue.y);
	Read(aValue.z);
	Read(aValue.w);
}

void BinarySerializer::Write(const glm::mat4& aValue)
{
	constexpr int matLength = glm::mat4::length() * glm::mat4::length();
	for (uint8_t index = 0; index < matLength; index++)
	{
		Write(glm::value_ptr(aValue)[index]);
	}
}

void BinarySerializer::Read(glm::mat4& aValue) const
{
	constexpr int matLength = glm::mat4::length() * glm::mat4::length();
	for (uint8_t index = 0; index < matLength; index++)
	{
		Read(glm::value_ptr(aValue)[index]);
	}
}