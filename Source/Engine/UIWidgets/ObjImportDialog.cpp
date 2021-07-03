#include "Precomp.h"
#include "ObjImportDialog.h"

#include "../Game.h"
#include "../Systems/ImGUI/ImGUISystem.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Model.h>

void ObjImportDialog::Draw(bool& aIsOpen)
{
	tbb::mutex::scoped_lock lock(Game::GetInstance()->GetImGUISystem().GetMutex());

	if (ImGui::Begin("Import Model", &aIsOpen))
	{
		DrawSourceInput(mySourceFile);
		ImGui::SameLine();
		const bool load = ImGui::Button("Load");

		if (load && !mySourceFile.empty())
		{
			myIsLoaded = ValidPath(mySourceFile) && myImporter.Load(mySourceFile);
			if (myIsLoaded)
			{
				mySavePaths.resize(myImporter.GetModelCount());
				for(size_t i=0; i<myImporter.GetModelCount(); i++)
				{
					mySavePaths[i] = myImporter.GetModelName(i);
				}
			}
		}

		if (myIsLoaded)
		{
			DrawObj();
		}
		else if(!mySourceFile.empty())
		{
			ImGui::Text("Failed to load OBJ!");
		}
	}
	ImGui::End();
}

constexpr bool ObjImportDialog::ValidPath(std::string_view aPath)
{
	// Not a real validity check, but it'll do for now
	return aPath.rfind(".obj") != std::string_view::npos;
}

void ObjImportDialog::DrawObj()
{
	for (size_t i = 0; i < myImporter.GetModelCount(); i++)
	{
		ImGui::PushID(static_cast<int>(i));

		ImGui::Separator();
		ImGui::Text("Model");

		const Handle<Model>& model = myImporter.GetModel(i);
		ImGui::LabelText("Name", myImporter.GetModelName(i).c_str());
		ImGui::LabelText("Vertices", "%zu", model->GetVertexCount());
		if (model->HasIndices())
		{
			ImGui::LabelText("Indices", "%zu", model->GetIndexCount());
		}
		else
		{
			ImGui::LabelText("Indices", "None");
		}

		{
			glm::vec3 min = model->GetAABBMin();
			ImGui::LabelText("AABB Min", "%f %f %f", min.x, min.y, min.z);
		}

		{
			glm::vec3 max = model->GetAABBMax();
			ImGui::LabelText("AABB Max", "%f %f %f", max.x, max.y, max.z);
		}

		std::string& savePath = mySavePaths[i];
		DrawSaveInput(savePath);
		if (ImGui::Button("Save"))
		{
			std::string fullSavePath = Resource::kAssetsFolder.CStr() + savePath;
			Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, model);
		}

		ImGui::PopID();
	}
}

void ObjImportDialog::DrawSourceInput(std::string& aPath)
{
	std::string tempInput = aPath;
	const bool changed = ImGui::InputText("Source", tempInput.data(), tempInput.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
		[](ImGuiInputTextCallbackData* aData)
	{
		std::string* valueStr = static_cast<std::string*>(aData->UserData);
		if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			valueStr->resize(aData->BufTextLen);
			aData->Buf = valueStr->data();
		}
		return 0;
	}, &tempInput);

	if (changed)
	{
		if (tempInput.size() > 1
			&& tempInput[0] == '"'
			&& tempInput[tempInput.size() - 1] == '"')
		{
			tempInput = tempInput.substr(1, tempInput.size() - 2);
		}

		if (tempInput != aPath)
		{
			aPath = tempInput;
		}
	}
}

void ObjImportDialog::DrawSaveInput(std::string& aPath)
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