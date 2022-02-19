#include "Precomp.h"
#include "GltfImportDialog.h"

#include "../Game.h"
#include "../Systems/ImGUI/ImGUISystem.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include "../Animation/AnimationClip.h"
#include "../Animation/Skeleton.h"

void GltfImportDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Import Model(GLTF)", &aIsOpen))
	{
		DrawSourceInput(mySourceFile);
		ImGui::SameLine();
		const bool load = ImGui::Button("Load");

		if (load && !mySourceFile.empty())
		{
			myIsLoaded = ValidPath(mySourceFile) && myImporter.Load(mySourceFile);
			if (myIsLoaded)
			{
				myModelNames.resize(myImporter.GetModelCount());
				for (size_t i = 0; i < myModelNames.size(); i++)
				{
					myModelNames[i] = myImporter.GetModelName(i);
				}

				myClipNames.resize(myImporter.GetClipCount());
				for (size_t i = 0; i < myClipNames.size(); i++)
				{
					myClipNames[i] = myImporter.GetClipName(i);
				}

				myTextureNames.resize(myImporter.GetTextureCount());
				for (size_t i = 0; i < myTextureNames.size(); i++)
				{
					myTextureNames[i] = myImporter.GetTextureName(i);
				}

				mySkeletonNames.resize(myImporter.GetSkeletonCount());
				for (size_t i = 0; i < mySkeletonNames.size(); i++)
				{
					mySkeletonNames[i] = myImporter.GetSkeletonName(i);
				}
			}
		}

		if (myIsLoaded)
		{
			DrawGltf();
		}
		else if (!mySourceFile.empty())
		{
			ImGui::Text("Failed to load GLTF!");
		}
	}
	ImGui::End();
}

constexpr bool GltfImportDialog::ValidPath(std::string_view aPath)
{
	// Not a real validity check, but it'll do for now
	return aPath.rfind(".gltf") != std::string_view::npos
		|| aPath.rfind(".glb") != std::string_view::npos;
}

void GltfImportDialog::DrawGltf()
{
	for (size_t i = 0; i < myImporter.GetModelCount(); i++)
	{
		DrawModel(i);
	}

	for (size_t i = 0; i < myImporter.GetTextureCount(); i++)
	{
		DrawTexture(i);
	}

	for (size_t i = 0; i < myImporter.GetSkeletonCount(); i++)
	{
		DrawSkeleton(i);
	}

	for (size_t i = 0; i < myImporter.GetClipCount(); i++)
	{
		DrawClip(i);
	}
}

void GltfImportDialog::DrawModel(size_t anIndex)
{
	ImGui::PushID("Model");
	ImGui::PushID(static_cast<int>(anIndex));

	ImGui::Separator();
	ImGui::Text("Model");

	const Handle<Model>& model = myImporter.GetModel(anIndex);
	ImGui::LabelText("Name", myImporter.GetModelName(anIndex).c_str());
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

	std::string& savePath = myModelNames[anIndex];
	DrawSaveInput(savePath);
	if (ImGui::Button("Save"))
	{
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + savePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, model);
	}

	ImGui::PopID();
	ImGui::PopID();
}

void GltfImportDialog::DrawClip(size_t anIndex)
{
	ImGui::PushID("Clip");
	ImGui::PushID(static_cast<int>(anIndex));

	ImGui::Separator();
	ImGui::Text("Animation Clip");

	const Handle<AnimationClip>& clip = myImporter.GetAnimClip(anIndex);
	ImGui::LabelText("Name", myImporter.GetClipName(anIndex).c_str());
	ImGui::LabelText("Length", "%fs", clip->GetLength());
	ImGui::LabelText("Track count", "%zu", clip->GetTracks().size());

	std::string& savePath = myClipNames[anIndex];
	DrawSaveInput(savePath);
	if (ImGui::Button("Save"))
	{
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + savePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, clip);
	}

	ImGui::PopID();
	ImGui::PopID();
}

void GltfImportDialog::DrawTexture(size_t anIndex)
{
	ImGui::PushID("Texture");
	ImGui::PushID(static_cast<int>(anIndex));

	ImGui::Separator();
	ImGui::Text("Texture");

	const Handle<Texture>& texture = myImporter.GetTexture(anIndex);
	ImGui::LabelText("Name", myImporter.GetTextureName(anIndex).c_str());
	ImGui::LabelText("Size", "%zux%zu", texture->GetWidth(), texture->GetHeight());
	switch (texture->GetFormat())
	{
	case Texture::Format::SNorm_R: ImGui::LabelText("Format", "sR"); break;
	case Texture::Format::SNorm_RG: ImGui::LabelText("Format", "sRG"); break;
	case Texture::Format::SNorm_RGB: ImGui::LabelText("Format", "sRGB"); break;
	case Texture::Format::SNorm_RGBA: ImGui::LabelText("Format", "sRGBA"); break;
	case Texture::Format::UNorm_R: ImGui::LabelText("Format", "R"); break;
	case Texture::Format::UNorm_RG: ImGui::LabelText("Format", "RG"); break;
	case Texture::Format::UNorm_RGB: ImGui::LabelText("Format", "RGB"); break;
	case Texture::Format::UNorm_RGBA: ImGui::LabelText("Format", "RGBA"); break;
	}

	// TODO: add preview draw

	std::string& savePath = myTextureNames[anIndex];
	DrawSaveInput(savePath);
	if (ImGui::Button("Save"))
	{
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + savePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, texture);
	}

	ImGui::PopID();
	ImGui::PopID();
}

void GltfImportDialog::DrawSkeleton(size_t anIndex)
{
	ImGui::PushID("Skeleton");
	ImGui::PushID(static_cast<int>(anIndex));

	ImGui::Separator();
	ImGui::Text("Skeleton");

	const Skeleton& skeleton = myImporter.GetSkeleton(anIndex);
	ImGui::LabelText("Name", myImporter.GetSkeletonName(anIndex).c_str());
	ImGui::LabelText("Bone count", "%zu", skeleton.GetBoneCount());

	ImGui::Text("Skeleton saving NYI");

	ImGui::PopID();
	ImGui::PopID();
}

void GltfImportDialog::DrawSourceInput(std::string& aPath)
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

void GltfImportDialog::DrawSaveInput(std::string& aPath)
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