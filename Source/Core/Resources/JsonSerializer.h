#pragma once

#include "Serializer.h"
#include <nlohmann/json.hpp>
#include <stack>

class JsonSerializer : public Serializer
{
	using Serializer::Serializer;
	void ReadFrom(const File& aFile) override final;
	void WriteTo(std::string& aBuffer) const override final;

	void SerializeImpl(std::string_view aName, const VariantType& aValue) override final;
	void SerializeImpl(size_t anIndex, const VariantType& aValue) override final;
	void DeserializeImpl(std::string_view aName, VariantType& aValue) const override final;
	void DeserializeImpl(size_t anIndex, VariantType& aValue) const override final;

	void BeginSerializeObjectImpl(std::string_view aName) override final;
	void BeginSerializeObjectImpl(size_t anIndex) override final;
	void EndSerializeObjectImpl(std::string_view aName) override final;
	void EndSerializeObjectImpl(size_t anIndex) override final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeObjectImpl(std::string_view aName) const override final;
	bool BeginDeserializeObjectImpl(size_t anIndex) const override final;
	void EndDeserializeObjectImpl(std::string_view aName) const override final;
	void EndDeserializeObjectImpl(size_t anIndex) const override final;

	void BeginSerializeArrayImpl(std::string_view aName, size_t aCount) override final;
	void BeginSerializeArrayImpl(size_t anIndex, size_t aCount) override final;
	void EndSerializeArrayImpl(std::string_view aName) override final;
	void EndSerializeArrayImpl(size_t anIndex) override final;
	// TODO: [[nodiscard]]
	bool BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const override final;
	bool BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const override final;
	void EndDeserializeArrayImpl(std::string_view aName) const override final;
	void EndDeserializeArrayImpl(size_t anIndex) const override final;

	mutable nlohmann::json myCurrObj;
	mutable std::stack<nlohmann::json> myObjStack;

	friend struct nlohmann::adl_serializer<ResourceProxy>;
	friend struct nlohmann::adl_serializer<VariantMap>;
};