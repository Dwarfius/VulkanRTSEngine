#include "Precomp.h"
#include "PipelineCreateDialog.h"

#include "../Game.h"
#include "../Systems/ImGUI/ImGUISystem.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Pipeline.h>

void PipelineCreateDialog::Draw(bool& aIsOpen)
{
	std::lock_guard lock(Game::GetInstance()->GetImGUISystem().GetMutex());

	if (ImGui::Begin("Import Pipeline", &aIsOpen))
	{
		DrawPipeline();
	}
	ImGui::End();
}

void PipelineCreateDialog::DrawPipeline()
{
	{
		using IndType = Pipeline::Type::UnderlyingType;
		IndType selectedInd = static_cast<IndType>(myPipelineType);
		if (ImGui::BeginCombo("Uniform Type", Pipeline::Type::kNames[selectedInd]))
		{
			for (IndType i = 0; i < Pipeline::Type::GetSize(); i++)
			{
				bool selected = selectedInd == i;
				if (ImGui::Selectable(Pipeline::Type::kNames[i], &selected))
				{
					selectedInd = i;
					myPipelineType = i;
				}
			}

			ImGui::EndCombo();
		}
	}

	static_assert(Shader::Type::GetSize() == kShaderTypes + 1, "Mismatch, please update constant!");
	for (size_t shaderType = 1; shaderType < Shader::Type::GetSize(); shaderType++)
	{
		std::string& path = myShaderPaths[shaderType - 1];
		ImGui::InputText(Shader::Type::kNames[shaderType], path.data(), path.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
			[](ImGuiInputTextCallbackData* aData)
		{
			std::string* valueStr = static_cast<std::string*>(aData->UserData);
			if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				valueStr->resize(aData->BufTextLen);
				aData->Buf = valueStr->data();
			}
			return 0;
		}, &path);
	}

	if (ImGui::Button("Add Descriptor"))
	{
		myDescriptors.push_back({});
	}
	ImGui::Indent();
	for(size_t i = 0; i<myDescriptors.size(); i++)
	{
		DrawDescriptor(myDescriptors[i]);

		if (ImGui::Button("Delete Descriptor"))
		{
			myDescriptors.erase(myDescriptors.begin() + i);
			i--;
		}
	}
	ImGui::Unindent();
	
	DrawSaveInput(mySavePath);
	if (ImGui::Button("Save"))
	{
		Handle<Pipeline> pipeline;
		pipeline->SetType(static_cast<Pipeline::Type>(myPipelineType));
		for (const std::string& shader : myShaderPaths)
		{
			if (!shader.empty())
			{
				pipeline->AddShader(shader);
			}
		}
		for (const Descriptor& descriptor : myDescriptors)
		{
			pipeline->AddDescriptor(descriptor);
		}
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + mySavePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, pipeline);
	}
}

void PipelineCreateDialog::DrawDescriptor(Descriptor& aDesc)
{
	{
		std::string adapter = aDesc.GetUniformAdapter();
		bool changed = ImGui::InputText("Adapter", adapter.data(), adapter.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
			[](ImGuiInputTextCallbackData* aData)
		{
			std::string* valueStr = static_cast<std::string*>(aData->UserData);
			if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				valueStr->resize(aData->BufTextLen);
				aData->Buf = valueStr->data();
			}
			return 0;
		}, &adapter);
		if (changed)
		{
			aDesc.SetUniformAdapter(adapter);
		}
	}

	bool needsRecompute = false;
	if (ImGui::Button("Add Uniform"))
	{
		aDesc.AddUniformType();
		needsRecompute = true;
	}
	ImGui::Indent();
	for (uint32_t i = 0; i < aDesc.GetUniformCount(); i++)
	{
		bool changed = false;

		Descriptor::UniformType uniform = aDesc.GetType(i);
		using IndType = Descriptor::UniformType::UnderlyingType;
		IndType selectedInd = static_cast<IndType>(uniform);
		if (ImGui::BeginCombo("Uniform Type", Descriptor::UniformType::kNames[selectedInd]))
		{
			for (IndType i = 0; i < Descriptor::UniformType::GetSize(); i++)
			{
				bool selected = selectedInd == i;
				if (ImGui::Selectable(Descriptor::UniformType::kNames[i], &selected))
				{
					changed = selectedInd != i;
					selectedInd = i;
					uniform = static_cast<Descriptor::UniformType>(selectedInd);
				}
			}

			ImGui::EndCombo();
		}

		uint32_t arraySize = aDesc.GetArraySize(i);
		changed |= ImGui::InputScalar("Array Size", ImGuiDataType_U32, &arraySize);

		if (changed)
		{
			aDesc.SetUniformType(i, uniform, arraySize);
			needsRecompute = true;
		}

		if (ImGui::Button("Delete Uniform"))
		{
			aDesc.RemoveUniformType(i);
			i--;
			needsRecompute = true;
		}
	}

	if (needsRecompute)
	{
		aDesc.RecomputeSize();
	}
	ImGui::Unindent();
}

void PipelineCreateDialog::DrawSaveInput(std::string& aPath)
{
	// we want to display a label and an InputText on the same line,
	// but Label doesn't have same vertical offset - so we apply the same
	// offset as the InputText does to our label to keep them aligned
	const float yAlignOffset = ImGui::GetStyle().FramePadding.y;

	float posY = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(posY + yAlignOffset);
	ImGui::Text(Resource::kAssetsFolder.CStr());
	ImGui::SameLine();
	ImGui::SetCursorPosY(posY);
	ImGui::InputText("Save As", aPath.data(), aPath.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
		[](ImGuiInputTextCallbackData* aData)
	{
		std::string* valueStr = static_cast<std::string*>(aData->UserData);
		if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			valueStr->resize(aData->BufTextLen);
			aData->Buf = valueStr->data();
		}
		return 0;
	}, &aPath);
}