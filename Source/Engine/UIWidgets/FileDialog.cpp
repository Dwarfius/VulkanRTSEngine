#include "Precomp.h"
#include "FileDialog.h"

#include "Game.h"
#include "Systems/ImGUI/ImGUISystem.h"

#include <Core/Resources/Resource.h>
#include <Core/Utils.h>

#include <filesystem>

bool FileDialog::Draw(std::string& aPath)
{
	std::filesystem::path rootDirPath = Resource::kAssetsFolder.CStr();
	std::error_code errorCode;
	std::filesystem::directory_entry rootDir(rootDirPath, errorCode);
	ASSERT_STR(!errorCode, "Unexpected, opening %s got error code %d", 
		rootDirPath.c_str(), errorCode.value());

	tbb::mutex::scoped_lock lock(Game::GetInstance()->GetImGUISystem().GetMutex());
	ImGui::Begin("File Dialog");
	if (ImGui::InputText("Regex Filter", myFilter, kRegexSize))
	{
		UpdateFilteredPaths(rootDir);
	}

	const std::filesystem::path& absRootPath = std::filesystem::absolute(rootDir.path(), errorCode);
	const std::string& rootDirU8Path = absRootPath.u8string();
	bool picked = false;
	if (!errorCode && ImGui::TreeNodeEx(rootDirU8Path.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (myFilter[0])
		{
			picked = DrawFilteredPaths(aPath);
		}
		else
		{
			picked = DrawHierarchy(rootDir, aPath);
		}
		
		ImGui::TreePop();
	}
	ImGui::End();

	return picked;
}

template<class TFunc>
void FileDialog::Traverse(std::filesystem::path aRoot, TFunc callback)
{
	std::error_code errorCode;
	std::filesystem::recursive_directory_iterator dirIter(aRoot, errorCode);
	if (errorCode)
	{
		return;
	}

	for (auto& entry : dirIter)
	{
		callback(entry, dirIter);
	}
}

void FileDialog::UpdateFilteredPaths(const std::filesystem::directory_entry& aRoot)
{
	myFilteredPaths.clear();

	if (myFilter[0] == 0)
	{
		return;
	}

	Traverse(aRoot, [&](const std::filesystem::directory_entry& entry,
		std::filesystem::recursive_directory_iterator&) {
		if (!entry.is_regular_file())
		{
			return;
		}

		std::error_code errorCode;
		const std::filesystem::path& filename = std::filesystem::relative(entry.path(), aRoot.path(), errorCode);
		ASSERT(!errorCode);
		const std::string& utf8Name = filename.u8string();
		if (Utils::Matches(utf8Name, myFilter))
		{
			myFilteredPaths.push_back(filename.u8string());
		}
	});
}

bool FileDialog::DrawFilteredPaths(std::string& aPath)
{
	bool picked = false;
	for (const auto& path : myFilteredPaths)
	{
		if (ImGui::Button(path.c_str()))
		{
			picked = true;
			aPath = path;
		}
	}
	return picked;
}

bool FileDialog::DrawHierarchy(const std::filesystem::directory_entry& aRoot, std::string& aPath)
{
	bool picked = false;
	int lastDepth = 0;
	std::filesystem::path rootDirPath = Resource::kAssetsFolder.CStr();

	Traverse(rootDirPath, [&](const std::filesystem::directory_entry& entry,
		std::filesystem::recursive_directory_iterator& dirIter) {
		const int depth = dirIter.depth();

		// close up previous branch if we went up
		for (int i = depth; i < lastDepth; i++)
		{
			ImGui::TreePop();
		}
		lastDepth = depth;

		if (entry.is_directory())
		{
			const std::filesystem::path& filename = entry.path().filename();
			const std::string& dirU8Path = filename.u8string();
			if (ImGui::TreeNode(dirU8Path.c_str()))
			{
				if (DrawFolder(entry, aPath))
				{
					picked = true;
				}

				// don't close the folder view here, as 
				// we might go deeper into the subtree
				if (!dirIter.recursion_pending())
				{
					ImGui::TreePop();
				}
			}
			else
			{
				// user didn't expand the tree node,
				// so avoid going into the subtree
				dirIter.disable_recursion_pending();
			}
		}
	});
	
	// close up a deep branch at the end, if there was one
	for (int i = lastDepth; i > 0; i--)
	{
		ImGui::TreePop();
	}

	if (DrawFolder(aRoot, aPath))
	{
		picked = true;
	}
	return picked;
}

bool FileDialog::DrawFolder(const std::filesystem::directory_entry& aDir, std::string& aPath) const
{
	std::error_code errCode;
	std::filesystem::directory_iterator iter(aDir, errCode);
	bool picked = false;
	bool startingOnNewLine = true;
	for (auto& file : iter)
	{
		if (!file.is_regular_file())
		{
			continue;
		}

		const std::string& filename = file.path().filename().u8string();
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 textSize = ImGui::CalcTextSize(filename.c_str());
		textSize.x += style.FramePadding.x * 2.0f;

		constexpr float kMinWidthToHave = 60;
		const float widthLeft = ImGui::GetWindowContentRegionMax().x - ImGui::GetCursorPosX();
		if (!startingOnNewLine && widthLeft <= textSize.x)
		{
			ImGui::NewLine();
		}
		
		if (ImGui::Button(filename.c_str()))
		{
			const std::filesystem::path& absPath = std::filesystem::absolute(file.path(), errCode);
			picked = true;
			aPath = absPath.u8string();
		}

		// this will advance the cursor to the updated position, 
		// on which we rely above
		ImGui::SameLine();
		startingOnNewLine = false;
	}
	ImGui::NewLine();
	return picked;
}