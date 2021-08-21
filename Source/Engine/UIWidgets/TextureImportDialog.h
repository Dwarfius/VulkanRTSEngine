#pragma once

#include <Core/RefCounted.h>

class Texture;

class TextureImportDialog
{
public:
	void Draw(bool& aIsOpen);
private:
	constexpr static bool ValidPath(std::string_view aPath);
	
	void DrawTexture();

	static void DrawSourceInput(std::string& aPath);
	static void DrawSaveInput(std::string& aPath);

	Handle<Texture> myTexture;
	std::string mySourceFile;
	std::string mySavePath;
	bool myIsLoaded = false;
};