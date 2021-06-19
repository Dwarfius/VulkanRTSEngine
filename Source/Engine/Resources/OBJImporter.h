#pragma once

#include <Core/RefCounted.h>

class Model;
class File;

// Imports all shapes declared in the .obj file as a single Model
class OBJImporter
{
public:
	bool Load(const std::string& aPath);
	bool Load(const File& aFile);
	bool Load(const std::vector<char>& aBuffer);

	size_t GetModelCount() const { return myModels.size(); }
	Handle<Model> GetModel(size_t aIndex) const { return myModels[aIndex]; }
	const std::string& GetModelName(size_t aIndex) const { return myModelNames[aIndex]; }

private:
	std::vector<Handle<Model>> myModels;
	std::vector<std::string> myModelNames;
};