#pragma once

#include "../Resources/OBJImporter.h"

class ObjImportDialog
{
public:
	void Draw(bool& aIsOpen);
private:
	constexpr static bool ValidPath(std::string_view aPath);
	
	void DrawObj();

	static void DrawSourceInput(std::string& aPath);
	static void DrawSaveInput(std::string& aPath);

	OBJImporter myImporter;
	std::string mySourceFile;
	std::vector<std::string> mySavePaths;
	bool myIsLoaded = false;
};