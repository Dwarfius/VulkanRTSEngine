#pragma once

#include <Core/RefCounted.h>

class Model;
class File;

// TODO: keep shapes as separate instead of pushing in the same one
// Imports all shapes declared in the .obj file as a single Model
class OBJImporter
{
public:
	bool Load(const std::string& aPath);
	bool Load(const File& aFile);
	bool Load(const std::vector<char>& aBuffer);

	const Handle<Model>& GetModel() { return myModel; }

private:
	Handle<Model> myModel;
};