#include "Precomp.h"
#include "TextureImportDialog.h"

#include "../Game.h"
#include "../Systems/ImGUI/ImGUISystem.h"
#include <Core/Resources/AssetTracker.h>
#include <Graphics/Resources/Texture.h>

void TextureImportDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Import Texture", &aIsOpen))
	{
		DrawSourceInput(mySourceFile);
		ImGui::SameLine();
		const bool load = ImGui::Button("Load");

		if (load && !mySourceFile.empty() && ValidPath(mySourceFile))
		{
			myTexture = Texture::LoadFromDisk(mySourceFile);
			myIsLoaded = myTexture.IsValid();
			if (myIsLoaded)
			{
				size_t lastSeparatorIndex = mySourceFile.rfind('/');
				if (lastSeparatorIndex == std::string::npos)
				{
					lastSeparatorIndex = mySourceFile.rfind('\\');
				}
				mySavePath = lastSeparatorIndex != std::string::npos ? 
					mySourceFile.substr(lastSeparatorIndex + 1) : mySourceFile;


				const size_t lastPeriodInd = mySavePath.rfind('.');
				if (lastPeriodInd != std::string::npos)
				{
					mySavePath = mySavePath.substr(0, lastPeriodInd);
				}
			}
		}

		if (myIsLoaded)
		{
			DrawTexture();
		}
		else if(!mySourceFile.empty())
		{
			ImGui::Text("Failed to load the texture!");
		}
	}
	ImGui::End();
}

constexpr bool TextureImportDialog::ValidPath(std::string_view aPath)
{
	// Not a real validity check, but it'll do for now
	return aPath.rfind(".png") != std::string_view::npos
		|| aPath.rfind(".jpg") != std::string_view::npos
		|| aPath.rfind(".jpeg") != std::string_view::npos
		|| aPath.rfind(".gif") != std::string_view::npos
		|| aPath.rfind(".bmp") != std::string_view::npos
		|| aPath.rfind(".tga") != std::string_view::npos;
}

void TextureImportDialog::DrawTexture()
{
	// read-only
	{
		const char* formatText = nullptr;
		switch (myTexture->GetFormat())
		{
		case Texture::Format::SNorm_R: formatText = "SNorm_R"; break;
		case Texture::Format::SNorm_RG: formatText = "SNorm_RG"; break;
		case Texture::Format::SNorm_RGB: formatText = "SNorm_RGB"; break;
		case Texture::Format::SNorm_RGBA: formatText = "SNorm_RGBA"; break;
		case Texture::Format::UNorm_R: formatText = "UNorm_R"; break;
		case Texture::Format::UNorm_RG: formatText = "UNorm_RG"; break;
		case Texture::Format::UNorm_RGB: formatText = "UNorm_RGB"; break;
		case Texture::Format::UNorm_RGBA: formatText = "UNorm_RGBA"; break;
		default: ASSERT(false);
		}
		ImGui::LabelText("Format", formatText);
	}

	ImGui::LabelText("Size", "%ux%u", myTexture->GetWidth(), myTexture->GetHeight());

	// change-able
	{
		using IndType = Texture::WrapMode::UnderlyingType;
		IndType selectedInd = static_cast<IndType>(myTexture->GetWrapMode());
		if (ImGui::BeginCombo("WrapMode", Texture::WrapMode::kNames[selectedInd]))
		{
			for (IndType i = 0; i < Texture::WrapMode::GetSize(); i++)
			{
				bool selected = selectedInd == i;
				if (ImGui::Selectable(Texture::WrapMode::kNames[i], &selected))
				{
					selectedInd = i;
					myTexture->SetWrapMode(static_cast<Texture::WrapMode>(i));
				}
			}

			ImGui::EndCombo();
		}
	}

	{
		using IndType = Texture::Filter::UnderlyingType;
		IndType selectedInd = static_cast<IndType>(myTexture->GetMinFilter());
		if (ImGui::BeginCombo("MinFilter", Texture::Filter::kNames[selectedInd]))
		{
			for (IndType i = 0; i < Texture::Filter::GetSize(); i++)
			{
				bool selected = selectedInd == i;
				if (ImGui::Selectable(Texture::Filter::kNames[i], &selected))
				{
					selectedInd = i;
					myTexture->SetMinFilter(static_cast<Texture::Filter>(i));
				}
			}

			ImGui::EndCombo();
		}
	}

	{
		using IndType = Texture::Filter::UnderlyingType;
		IndType selectedInd = static_cast<IndType>(myTexture->GetMagFilter());
		if (ImGui::BeginCombo("MagFilter", Texture::Filter::kNames[selectedInd]))
		{
			for (IndType i = Texture::Filter::Nearest; i <= Texture::Filter::Linear; i++)
			{
				bool selected = selectedInd == i;
				if (ImGui::Selectable(Texture::Filter::kNames[i], &selected))
				{
					selectedInd = i;
					myTexture->SetMagFilter(static_cast<Texture::Filter>(i));
				}
			}

			ImGui::EndCombo();
		}
	}

	{
		bool usingMipMaps = myTexture->IsUsingMipMaps();
		if (ImGui::Checkbox("MipMaps enabled", &usingMipMaps))
		{
			myTexture->EnableMipMaps(usingMipMaps);
		}
	}

	DrawSaveInput(mySavePath);
	if (ImGui::Button("Save"))
	{
		std::string fullSavePath = Resource::kAssetsFolder.CStr() + mySavePath;
		Game::GetInstance()->GetAssetTracker().SaveAndTrack(fullSavePath, myTexture);
	}
}

void TextureImportDialog::DrawSourceInput(std::string& aPath)
{
	std::string tempInput = aPath;
	const bool changed = ImGui::InputText("Source", tempInput);

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

void TextureImportDialog::DrawSaveInput(std::string& aPath)
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
	ImGui::InputText("Save As", aPath);
}