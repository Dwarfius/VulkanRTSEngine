#include "Precomp.h"
#include "ImGUISerializer.h"

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

void ImGUISerializer::SerializeExternal(std::string_view, std::vector<char>&)
{
	// NOOP for now
}

void ImGUISerializer::SerializeImpl(std::string_view, const VariantType&)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::SerializeImpl(size_t, const VariantType&)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::DeserializeImpl(std::string_view aName, VariantType& aValue) const
{
	std::visit([&](auto&& aVarValue) {
		Display(aName, aVarValue);
	}, aValue);
}

void ImGUISerializer::DeserializeImpl(size_t anIndex, VariantType& aValue) const
{
	std::visit([&](auto&& aVarValue) {
		char indexLbl[16] = { 0 };
		Utils::StringFormat(indexLbl, "%llu", anIndex);
		Display(indexLbl, aVarValue);
	}, aValue);
}

void ImGUISerializer::BeginSerializeObjectImpl(std::string_view)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::BeginSerializeObjectImpl(size_t)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::EndSerializeObjectImpl(std::string_view)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::EndSerializeObjectImpl(size_t)
{
	// NOOP - this serializer is always reading!
}

bool ImGUISerializer::BeginDeserializeObjectImpl(std::string_view aName) const
{
	return ImGui::TreeNode(aName.data());
}

bool ImGUISerializer::BeginDeserializeObjectImpl(size_t anIndex) const
{
	char indexLbl[16] = { 0 };
	Utils::StringFormat(indexLbl, "%zu", anIndex);
	return ImGui::TreeNode(indexLbl);
}

void ImGUISerializer::EndDeserializeObjectImpl(std::string_view) const
{
	ImGui::TreePop();
}

void ImGUISerializer::EndDeserializeObjectImpl(size_t) const
{
	ImGui::TreePop();
}

void ImGUISerializer::BeginSerializeArrayImpl(std::string_view aName, size_t aCount)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::BeginSerializeArrayImpl(size_t anIndex, size_t aCount)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::EndSerializeArrayImpl(std::string_view aName)
{
	// NOOP - this serializer is always reading!
}

void ImGUISerializer::EndSerializeArrayImpl(size_t anIndex)
{
	// NOOP - this serializer is always reading!
}

bool ImGUISerializer::BeginDeserializeArrayImpl(std::string_view aName, size_t& aCount) const
{
	if (aCount >= kArrayTooBigLimit)
	{
		ImGui::LabelText(aName.data(), "Large array[%zu]", aCount);
		return false;
	}

	const bool isOpen = ImGui::TreeNode(aName.data());
	if (isOpen)
	{
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

bool ImGUISerializer::BeginDeserializeArrayImpl(size_t anIndex, size_t& aCount) const
{
	char indexLbl[16] = { 0 };
	Utils::StringFormat(indexLbl, "%zu", anIndex);

	if (aCount >= kArrayTooBigLimit)
	{
		ImGui::LabelText(indexLbl, "Large array[%zu]", aCount);
		return false;
	}
	
	const bool isOpen = ImGui::TreeNode(indexLbl);
	if (isOpen)
	{
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

void ImGUISerializer::EndDeserializeArrayImpl(std::string_view aName) const
{
	ImGui::TreePop();
}

void ImGUISerializer::EndDeserializeArrayImpl(size_t anIndex) const
{
	ImGui::TreePop();
}

void ImGUISerializer::Display(std::string_view aLabel, bool& aValue) const
{
	ImGui::Checkbox(aLabel.data(), &aValue);
}

void ImGUISerializer::Display(std::string_view aLabel, uint64_t& aValue) const
{
	ImGui::InputScalar(aLabel.data(), ImGuiDataType_U64, &aValue);
}

void ImGUISerializer::Display(std::string_view aLabel, int64_t& aValue) const
{
	ImGui::InputScalar(aLabel.data(), ImGuiDataType_S64, &aValue);
}

void ImGUISerializer::Display(std::string_view aLabel, float& aValue) const
{
	ImGui::InputFloat(aLabel.data(), &aValue);
}

void ImGUISerializer::Display(std::string_view aLabel, std::string& aValue) const
{
	ImGui::InputText(aLabel.data(), aValue.data(), aValue.capacity() + 1, ImGuiInputTextFlags_CallbackResize, 
		[](ImGuiInputTextCallbackData* aData) 
	{
		std::string* valueStr = static_cast<std::string*>(aData->UserData);
		if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			valueStr->resize(aData->BufTextLen);
			aData->Buf = valueStr->data();
		}
		return 0;
	}, &aValue);
}

void ImGUISerializer::Display(std::string_view aLabel, VariantMap& aValue) const
{
	ASSERT_STR(false, "NYI");
}

void ImGUISerializer::Display(std::string_view aLabel, ResourceProxy& aValue) const
{
	// TODO: expand this
	// for now, just displaying the path
	ImGui::LabelText(aLabel.data(), aValue.myPath.c_str());
}

void ImGUISerializer::Display(std::string_view aLabel, glm::vec2& aValue) const
{
	ImGui::InputFloat2(aLabel.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Display(std::string_view aLabel, glm::vec3& aValue) const
{
	ImGui::InputFloat3(aLabel.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Display(std::string_view aLabel, glm::vec4& aValue) const
{
	ImGui::InputFloat4(aLabel.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Display(std::string_view aLabel, glm::quat& aValue) const
{
	ImGui::InputFloat4(aLabel.data(), glm::value_ptr(aValue));
}

void ImGUISerializer::Display(std::string_view aLabel, glm::mat4& aValue) const
{
	for (char i = 0; i < 4; i++)
	{
		ImGui::InputFloat4(aLabel.data(), glm::value_ptr(aValue[i]));
	}
}