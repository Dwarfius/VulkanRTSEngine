#pragma once

#include <Core/Resources/Resource.h>

class Model;

// TODO: keep shapes as separate instead of pushing in the same one
// Imports all shapes declared in the .obj file as a single Model
class OBJImporter : public Resource
{
public:
	using Resource::Resource;

	const Handle<Model>& GetModel() { return myModel; }

private:
	// Determines whether this resource loads a descriptor via Serializer or a raw resorce
	bool UsesDescriptor() const final { return false; };
	void OnLoad(const std::vector<char>& aBuffer, AssetTracker&) final;

	Handle<Model> myModel;
};