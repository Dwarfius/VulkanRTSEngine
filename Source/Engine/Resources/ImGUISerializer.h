#pragma once

#include <Core/Resources/Serializer.h>

class ImGUISerializer : public Serializer
{
public:
	ImGUISerializer(AssetTracker& anAssetTracker);

	void ReadFrom(const std::vector<char>& aBuffer) final;
	void WriteTo(std::vector<char>& aBuffer) const final;

	void SerializeExternal(std::string_view aFile, std::vector<char>& aBlob) final;

private:
	static constexpr size_t kArrayTooBigLimit = 100;
	void SerializeImpl(std::string_view aName, const VariantType& aValue) final;
	void SerializeImpl(size_t anIndex, const VariantType& aValue) final;
	void DeserializeImpl(std::string_view aName, VariantType& aValue) const final;
	void DeserializeImpl(size_t anIndex, VariantType& aValue) const final;
	
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

private:
	void Display(std::string_view aLabel, bool& aValue) const;
	void Display(std::string_view aLabel, uint64_t& aValue) const;
	void Display(std::string_view aLabel, int64_t& aValue) const;
	void Display(std::string_view aLabel, float& aValue) const;
	void Display(std::string_view aLabel, std::string& aValue) const;
	void Display(std::string_view aLabel, VariantMap& aValue) const;
	void Display(std::string_view aLabel, ResourceProxy& aValue) const;
	void Display(std::string_view aLabel, glm::vec2& aValue) const;
	void Display(std::string_view aLabel, glm::vec3& aValue) const;
	void Display(std::string_view aLabel, glm::vec4& aValue) const;
	void Display(std::string_view aLabel, glm::quat& aValue) const;
	void Display(std::string_view aLabel, glm::mat4& aValue) const;
};