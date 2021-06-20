#pragma once

#include "../Resources/GLTFImporter.h"

class GltfImportDialog
{
public:
	void Draw(bool& aIsOpen);
private:
	constexpr static bool ValidPath(std::string_view aPath);

	void DrawGltf();
	void DrawModel(size_t anIndex);
	void DrawClip(size_t anIndex);
	void DrawTexture(size_t anIndex);
	void DrawSkeleton(size_t anIndex);

	bool DrawSourceInput(std::string& aPath);
	void DrawSaveInput(std::string& aPath);

	GLTFImporter myImporter;
	std::string mySourceFile;
	std::vector<std::string> myModelNames;
	std::vector<std::string> myClipNames;
	std::vector<std::string> myTextureNames;
	std::vector<std::string> mySkeletonNames;
	bool myIsLoaded = false;
};