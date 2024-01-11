#include "Precomp.h"
#include "ImGUISerializer.h"

#include <Core/Utils.h>

ImGUISerializer::ImGUISerializer(AssetTracker& anAssetTracker)
	: Serializer(anAssetTracker, true)
{
}

void ImGUISerializer::ReadFrom(const std::vector<char>&)
{
	ASSERT_STR(false, "Not supported!");
}

void ImGUISerializer::WriteTo(std::vector<char>&) const
{
	ASSERT_STR(false, "Not supported!");
}

void ImGUISerializer::Serialize(std::string_view aName, bool& aValue) 
{
	ImGui::Checkbox(aName.data(), &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, uint8_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_U8, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, uint16_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_U16, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, uint32_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_U32, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, uint64_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_U64, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, int8_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_S8, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, int16_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_S16, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, int32_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_S32, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, int64_t& aValue)
{
	ImGui::InputScalar(aName.data(), ImGuiDataType_S64, &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, float& aValue)
{
	ImGui::InputFloat(aName.data(), &aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, std::string& aValue)
{
	ImGui::InputText(aName, aValue);
}

void ImGUISerializer::Serialize(std::string_view aName, glm::vec2& aValue)
{
	ImGui::InputFloat2(aName.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Serialize(std::string_view aName, glm::vec3& aValue)
{
	ImGui::InputFloat3(aName.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Serialize(std::string_view aName, glm::vec4& aValue)
{
	ImGui::InputFloat4(aName.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Serialize(std::string_view aName, glm::quat& aValue)
{
	ImGui::InputFloat4(aName.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Serialize(std::string_view aName, glm::mat4& aValue)
{
	for (char i = 0; i < 4; i++)
	{
		ImGui::InputFloat4(aName.data(), glm::value_ptr(aValue[i]));
	}
}

void ImGUISerializer::SerializeExternal(std::string_view, std::vector<char>&)
{
	// NOOP for now
}

bool ImGUISerializer::BeginSerializeObjectImpl(std::string_view aName)
{
	const bool isPartOfArray = aName.empty();
	if (isPartOfArray)
	{
		State& state = myStateStack.top();

		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", state.myCurrIndex);
		state.myCurrIndex++;

		return ImGui::TreeNode(indexLbl);
	}
	else
	{
		return ImGui::TreeNode(aName.data());
	}
}

void ImGUISerializer::EndSerializeObjectImpl(std::string_view)
{
	ImGui::TreePop();
}

bool ImGUISerializer::BeginSerializeArrayImpl(std::string_view aName, size_t& aCount)
{
	if (aCount >= kArrayTooBigLimit)
	{
		ImGui::LabelText(aName.data(), "Large array[%zu]", aCount);
		return false;
	}

	const bool isOpen = ImGui::TreeNode(aName.data());
	if (isOpen)
	{
		myStateStack.push({});

		if (ImGui::Button("Add new"))
		{
			aCount++;
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			aCount = 0;
		}
	}
	return isOpen;
}

void ImGUISerializer::EndSerializeArrayImpl(std::string_view aName)
{
	ImGui::TreePop();
	myStateStack.pop();
}

void ImGUISerializer::SerializeSpan(bool* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::Checkbox(indexLbl, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(uint8_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_U8, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(uint16_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_U16, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(uint32_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_U32, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(uint64_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_U64, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(int8_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_S8, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(int16_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_S16, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(int32_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_S32, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(int64_t* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputScalar(indexLbl, ImGuiDataType_S64, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(float* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputFloat(indexLbl, aValues + i);
	}
}

void ImGUISerializer::SerializeSpan(std::string* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);

		std::string& value = aValues[i];
		ImGui::InputText(indexLbl, value);
	}
}

void ImGUISerializer::SerializeSpan(glm::vec2* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputFloat2(indexLbl, glm::value_ptr(aValues[i]));
	}
}

void ImGUISerializer::SerializeSpan(glm::vec3* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputFloat3(indexLbl, glm::value_ptr(aValues[i]));
	}
}

void ImGUISerializer::SerializeSpan(glm::vec4* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputFloat4(indexLbl, glm::value_ptr(aValues[i]));
	}
}

void ImGUISerializer::SerializeSpan(glm::quat* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		ImGui::InputFloat4(indexLbl, glm::value_ptr(aValues[i]));
	}
}

void ImGUISerializer::SerializeSpan(glm::mat4* aValues, size_t aSize)
{
	for (size_t i = 0; i < aSize; i++)
	{
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", i);
		for (char m = 0; m < 4; m++)
		{
			ImGui::InputFloat4(indexLbl, glm::value_ptr(aValues[i][m]));
		}
	}
}

void ImGUISerializer::SerializeEnum(std::string_view aName, size_t& anEnumValue, const char* const* aNames, size_t)
{
	const bool isPartOfArray = aName.empty();
	if (isPartOfArray)
	{
		State& state = myStateStack.top();
		
		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", state.myCurrIndex);
		state.myCurrIndex++;

		ImGui::LabelText(indexLbl, aNames[anEnumValue]);
	}
	else
	{
		ImGui::LabelText(aName.data(), aNames[anEnumValue]);
	}
}

void ImGUISerializer::SerializeResource(std::string_view aName, ResourceProxy& aValue)
{
	// TODO: expand this
	// for now, just displaying the path
	const bool isPartOfArray = aName.empty();
	if (isPartOfArray)
	{
		State& state = myStateStack.top();

		char indexLbl[16];
		Utils::StringFormat(indexLbl, "{}", state.myCurrIndex);
		state.myCurrIndex++;

		ImGui::LabelText(indexLbl, aValue.myPath.c_str());
		
	}
	else
	{
		ImGui::LabelText(aName.data(), aValue.myPath.c_str());
	}
}