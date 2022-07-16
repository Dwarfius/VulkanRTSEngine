#include "Precomp.h"
#include "FileDialog.h"

#include <filesystem>
#include <Core/Resources/Resource.h>

void FileDialog::Draw(std::string_view anExt)
{
	if (ImGui::Begin("Assets"))
	{
		ImGui::LabelText("Path", Resource::kAssetsFolder.CStr());
		if (ImGui::Button("Scan") || !myHadInitScan)
		{
			myFiles.clear();

			namespace fs = std::filesystem;
			std::error_code errCode;
			fs::recursive_directory_iterator iter(Resource::kAssetsFolder.CStr(), errCode);
			for (fs::path path : iter)
			{
				std::string ext = path.extension().string();
				if (!ext.ends_with(anExt))
				{
					continue;
				}

				File file{
					path.string().substr(Resource::kAssetsFolder.GetLength() - 1),
					ext
				};
				myFiles.emplace_back(std::move(file));
			}
			std::ranges::sort(myFiles, std::less<File>());
			myHadInitScan = true;
		}
		ImGui::Separator();

		ImGui::InputText("Filter", myFilter, kMaxFilterSize);
		if (!myFiles.empty())
		{
			std::string_view currExt;
			for (const File& file : myFiles)
			{
				if (myFilter[0]
					&& file.myPath.find(myFilter) == std::string::npos)
				{
					continue;
				}

				if (file.myType != currExt)
				{
					ImGui::Text(file.myType.c_str());
					currExt = file.myType;
				}

				if (ImGui::Button(file.myPath.c_str()))
				{
					myPickedFile = &file;
				}
			}
		}
	}
	ImGui::End();
}

bool FileDialog::GetPickedFile(File& aFile)
{
	if (myPickedFile)
	{
		aFile = *myPickedFile;
		myPickedFile = nullptr;
		return true;
	}
	return false;
}