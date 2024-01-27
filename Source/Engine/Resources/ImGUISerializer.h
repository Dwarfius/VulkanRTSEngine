#pragma once

#include <Core/Resources/Serializer.h>
#include <stack>

class ImGUISerializer final : public Serializer
{
	static constexpr size_t kArrayTooBigLimit = 100;
public:
	ImGUISerializer(AssetTracker& anAssetTracker);

	void ReadFrom(const std::vector<char>& aBuffer) override;
	void WriteTo(std::vector<char>& aBuffer) const override;

private:
	void Serialize(std::string_view aName, bool& aValue) override;
	void Serialize(std::string_view aName, uint8_t& aValue) override;
	void Serialize(std::string_view aName, uint16_t& aValue) override;
	void Serialize(std::string_view aName, uint32_t& aValue) override;
	void Serialize(std::string_view aName, uint64_t& aValue) override;
	void Serialize(std::string_view aName, int8_t& aValue) override;
	void Serialize(std::string_view aName, int16_t& aValue) override;
	void Serialize(std::string_view aName, int32_t& aValue) override;
	void Serialize(std::string_view aName, int64_t& aValue) override;
	void Serialize(std::string_view aName, float& aValue) override;
	void Serialize(std::string_view aName, std::string& aValue) override;
	void Serialize(std::string_view aName, glm::vec2& aValue) override;
	void Serialize(std::string_view aName, glm::vec3& aValue) override;
	void Serialize(std::string_view aName, glm::vec4& aValue) override;
	void Serialize(std::string_view aName, glm::quat& aValue) override;
	void Serialize(std::string_view aName, glm::mat4& aValue) override;

	void SerializeExternal(std::string_view aFile, std::vector<char>& aBlob, Resource::Id anId) override;

	bool BeginSerializeObjectImpl(std::string_view aName) override;
	void EndSerializeObjectImpl(std::string_view aName) override;

	bool BeginSerializeArrayImpl(std::string_view aName, size_t& aCount) override;
	void EndSerializeArrayImpl(std::string_view aName) override;

	void SerializeSpan(bool* aValues, size_t aSize) override;
	void SerializeSpan(uint8_t* aValues, size_t aSize) override;
	void SerializeSpan(uint16_t* aValues, size_t aSize) override;
	void SerializeSpan(uint32_t* aValues, size_t aSize) override;
	void SerializeSpan(uint64_t* aValues, size_t aSize) override;
	void SerializeSpan(int8_t* aValues, size_t aSize) override;
	void SerializeSpan(int16_t* aValues, size_t aSize) override;
	void SerializeSpan(int32_t* aValues, size_t aSize) override;
	void SerializeSpan(int64_t* aValues, size_t aSize) override;
	void SerializeSpan(float* aValues, size_t aSize) override;
	void SerializeSpan(std::string* aValues, size_t aSize) override;
	void SerializeSpan(glm::vec2* aValues, size_t aSize) override;
	void SerializeSpan(glm::vec3* aValues, size_t aSize) override;
	void SerializeSpan(glm::vec4* aValues, size_t aSize) override;
	void SerializeSpan(glm::quat* aValues, size_t aSize) override;
	void SerializeSpan(glm::mat4* aValues, size_t aSize) override;

	void SerializeResource(std::string_view aName, ResourceProxy& aValue) override;
	void SerializeEnum(std::string_view aName, size_t& anEnumValue, const char* const* aNames, size_t aNamesLength) override;

	struct State
	{
		size_t myCurrIndex = 0;
	};
	std::stack<State> myStateStack;
};