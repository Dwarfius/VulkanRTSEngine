#include "Precomp.h"
#include "ShaderCreateDialog.h"

#include "../Game.h"
#include "../Systems/ImGUI/ImGUISystem.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Shader.h>

void ShaderCreateDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Import Shader", &aIsOpen))
	{
		DrawShader();
	}
	ImGui::End();
}

void ShaderCreateDialog::DrawShader()
{
	ImGui::InputTextMultiline("Source", mySource.data(), mySource.capacity() + 1, ImVec2(0,0), ImGuiInputTextFlags_CallbackResize,
		[](ImGuiInputTextCallbackData* aData)
	{
		std::string* valueStr = static_cast<std::string*>(aData->UserData);
		if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			valueStr->resize(aData->BufTextLen);
			aData->Buf = valueStr->data();
		}
		return 0;
	}, &mySource);
	
	{
		using IndType = Shader::Type::UnderlyingType;
		IndType selectedInd = static_cast<IndType>(myShaderType);
		if (ImGui::BeginCombo("WrapMode", Shader::Type::kNames[selectedInd]))
		{
			for (IndType i = 0; i < Shader::Type::GetSize(); i++)
			{
				bool selected = selectedInd == i;
				if (ImGui::Selectable(Shader::Type::kNames[i], &selected))
				{
					selectedInd = i;
					myShaderType = selectedInd;
				}
			}

			ImGui::EndCombo();
		}
	}

	DrawSaveInput(mySavePath);
	if (ImGui::Button("Save"))
	{
		Handle<Shader> shader = new Shader();
		shader->Upload(mySource.data(), mySource.size());
		shader->SetType(static_cast<Shader::Type>(myShaderType));
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + mySavePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, shader);
	}
}

void ShaderCreateDialog::DrawSaveInput(std::string& aPath)
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