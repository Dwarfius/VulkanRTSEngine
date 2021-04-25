#pragma once

namespace std::filesystem 
{
	class directory_entry;
	class path;
}

class FileDialog
{
public:
	bool Draw(std::string& aPath);

private:
	template<class TFunc>
	void Traverse(std::filesystem::path aRoot, TFunc callback);
	void UpdateFilteredPaths(const std::filesystem::directory_entry& aRoot);
	bool DrawFilteredPaths(std::string& aPath);
	bool DrawHierarchy(const std::filesystem::directory_entry& aRoot, std::string& aPath);
	bool DrawFolder(const std::filesystem::directory_entry& aDir, std::string& aPath) const;

	constexpr static size_t kRegexSize = 50;
	char myFilter[kRegexSize] = { 0 };
	std::vector<std::string> myFilteredPaths;
};

