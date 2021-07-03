#pragma once

#include "Serializer.h"
#include <nlohmann/json.hpp>
#include <stack>

class JsonSerializer final : public Serializer
{
	using Serializer::Serializer;

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
	bool BeginDeserializeObjectImpl(size_t anIndex) const final;
	void EndDeserializeObjectImpl(std::string_view aName) const final;
	void EndDeserializeObjectImpl(size_t anIndex) const final;

	void BeginSerializeArrayImpl(std::string_view aName, size_t aCount) final;
	void BeginSerializeArrayImpl(size_t anIndex, size_t aCount) final;
	void EndSerializeArrayImpl(std::string_view aName) final;
	void EndSerializeArrayImpl(size_t anIndex) final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const final;
	bool BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const final;
	void EndDeserializeArrayImpl(std::string_view aName) const final;
	void EndDeserializeArrayImpl(size_t anIndex) const final;

	mutable nlohmann::json myCurrObj = nlohmann::json::object_t();
	mutable std::stack<nlohmann::json> myObjStack;

	friend struct nlohmann::adl_serializer<ResourceProxy>;
	friend struct nlohmann::adl_serializer<VariantMap>;
};