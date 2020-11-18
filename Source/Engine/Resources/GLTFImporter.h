#pragma once

#include <Core/Resources/Resource.h>

class Model;

// Imports all shapes declared in the .gltf file as a single Model
class GLTFImporter : public Resource
{
public:
	using Resource::Resource;
	GLTFImporter(Id anId, const std::string& aPath);

	static constexpr StaticString kDir = Resource::AssetsFolder + "objects/";

	const Handle<Model>& GetModel() { return myModel; }

private:
	// Determines whether this resource loads a descriptor via Serializer or a raw resorce
	bool UsesDescriptor() const override final { return false; };
	void OnLoad(const File& aFile) override final;

	Handle<Model> myModel;
};