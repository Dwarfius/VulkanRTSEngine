#pragma once

#include "Serializer.h"

// Linearly serializes data into a single, flattened binary blob
// Binary blob will not contain metadata (no names, no separators,
// no type info), but will contain array size
// Internal binary serialization is little-endian
class BinarySerializer : public Serializer
{
	using Serializer::Serializer;

	// Serializer Interface
public:
	void ReadFrom(const std::vector<char>& aBuffer) final;
	void WriteTo(std::vector<char>& aBuffer) const final;

	void SerializeExternal(std::string_view aFile, std::vector<char>& aBlob) final;

private:
	void SerializeImpl(std::string_view aName, const VariantType& aValue) final;
	void SerializeImpl(size_t anIndex, const VariantType& aValue) final;
	void DeserializeImpl(std::string_view aName, VariantType& aValue) const final;
	void DeserializeImpl(size_t anIndex, VariantType& aValue) const final;

	void SerializeEnumImpl(std::string_view aName, size_t anEnumValue, const char* const* aNames, size_t aNamesLength) final;
	void SerializeEnumImpl(size_t anIndex, size_t anEnumValue, const char* const* aNames, size_t aNamesLength) final;
	void DeserializeEnumImpl(std::string_view aName, size_t& anEnumValue, const char* const* aNames, size_t aNamesLength) const final;
	void DeserializeEnumImpl(size_t anIndex, size_t& anEnumValue, const char* const* aNames, size_t aNamesLength) const final;

	void BeginSerializeObjectImpl(std::string_view aName) final;
	void BeginSerializeObjectImpl(size_t anIndex) final;
	void EndSerializeObjectImpl(std::string_view aName) final;
	void EndSerializeObjectImpl(size_t anIndex) final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeObjectImpl(std::string_view aName) const final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeObjectImpl(size_t anIndex) const final;
	void EndDeserializeObjectImpl(std::string_view aName) const final;
	void EndDeserializeObjectImpl(size_t anIndex) const final;

	void BeginSerializeArrayImpl(std::string_view aName, size_t aCount) final;
	void BeginSerializeArrayImpl(size_t anIndex, size_t aCount) final;
	void EndSerializeArrayImpl(std::string_view aName) final;
	void EndSerializeArrayImpl(size_t anIndex) final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const final;
	void EndDeserializeArrayImpl(std::string_view aName) const final;
	void EndDeserializeArrayImpl(size_t anIndex) const final;

	// Read/Write Impl
private:
	void Write(bool aValue);
	void Read(bool& aValue) const;
	void Write(char aValue);
	void Read(char& aValue) const;
	void Write(uint32_t aValue);
	void Read(uint32_t& aValue) const;
	void Write(int32_t aValue);
	void Read(int32_t& aValue) const;
	void Write(uint64_t aValue);
	void Read(uint64_t& aValue) const;
	void Write(int64_t aValue);
	void Read(int64_t& aValue) const;
	void Write(float aValue);
	void Read(float& aValue) const;
	void Write(const std::string& aValue);
	void Read(std::string& aValue) const;
	void Write(const ResourceProxy& aValue);
	void Read(ResourceProxy& aValue) const;
	void Write(const glm::vec2& aValue);
	void Read(glm::vec2& aValue) const;
	void Write(const glm::vec3& aValue);
	void Read(glm::vec3& aValue) const;
	void Write(const glm::vec4& aValue);
	void Read(glm::vec4& aValue) const;
	void Write(const glm::quat& aValue);
	void Read(glm::quat& aValue) const;
	void Write(const glm::mat4& aValue);
	void Read(glm::mat4& aValue) const;

	std::vector<char> myBuffer;
	mutable size_t myIndex;
};