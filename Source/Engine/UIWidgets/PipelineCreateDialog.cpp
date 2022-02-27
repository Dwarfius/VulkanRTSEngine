#include "Precomp.h"
#include "PipelineCreateDialog.h"

#include "../Game.h"
#include "../Systems/ImGUI/ImGUISystem.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Pipeline.h>

void PipelineCreateDialog::Draw(bool& aIsOpen)
{
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
		if (ImGui::BeginCombo("Pipeline Type", Pipeline::Type::kNames[selectedInd]))
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
	
	ImGui::Indent();
	for (size_t i = 0; i < myAdapters.size(); i++)
	{
		ImGui::PushID(i);
		DrawAdapter(myAdapters[i]);

		if (ImGui::Button("Delete Adapter"))
		{
			myAdapters.erase(myAdapters.begin() + i);
			i--;
		}
		ImGui::PopID();
	}

	if (ImGui::Button("Add Adapter"))
	{
		myAdapters.push_back({});
	}
	ImGui::Unindent();
	
	DrawSaveInput(mySavePath);
	if (ImGui::Button("Save"))
	{
		Handle<Pipeline> pipeline = new Pipeline();
		pipeline->SetType(static_cast<Pipeline::Type>(myPipelineType));
		for (const std::string& shader : myShaderPaths)
		{
			if (!shader.empty())
			{
				pipeline->AddShader(shader);
			}
		}
		for (const std::string& adapter : myAdapters)
		{
			if (!adapter.empty())
			{
				pipeline->AddAdapter(adapter);
			}
		}
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + mySavePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, pipeline);
	}
}

void PipelineCreateDialog::DrawAdapter(std::string& anAdapter)
{
	ImGui::InputText("Adapter", anAdapter.data(), anAdapter.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
		[](ImGuiInputTextCallbackData* aData)
	{
		std::string* valueStr = static_cast<std::string*>(aData->UserData);
		if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			valueStr->resize(aData->BufTextLen);
			aData->Buf = valueStr->data();
		}
		return 0;
	}, &anAdapter);
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