#pragma once

#include "Serializer.h"
#include <nlohmann/json.hpp> // TODO: would be nice to Pimpl it without an allocation per construction

class JsonSerializer : public Serializer
{
	using Serializer::Serializer;
	void ReadFrom(const File& aFile) override final;
	void WriteTo(std::string& aBuffer) const override final;
	void SerializeImpl(const std::string_view& aName, const VariantType& aValue) override final;
	void SerializeImpl(const std::string_view& aName, const VecVariantType& aValue, const VariantType& aHint) override final;
	void DeserializeImpl(const std::string_view& aName, VariantType& aValue) override final;
	void DeserializeImpl(const std::string_view& aName, VecVariantType& aValue, const VariantType& aHint) override final;

	nlohmann::json myJsonObj;

	friend struct nlohmann::adl_serializer<ResourceProxy>;
};