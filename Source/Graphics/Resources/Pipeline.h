#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/IPipeline.h"
#include "../Descriptor.h"
#include "../Resources/Shader.h"

// A base class describing a generic pipeline
class Pipeline : public Resource, public IPipeline
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "pipelines/";

public:
	Pipeline();
	Pipeline(Resource::Id anId, const std::string& aPath);

	size_t GetDescriptorCount() const override final { return myDescriptors.size(); }
	Handle<Descriptor> GetDescriptor(size_t anIndex) const override final { return myDescriptors[anIndex]; }

	void AddShader(const std::string& aShader) { myShaders.push_back(aShader); }

	void AddDescriptor(Handle<Descriptor> aDescriptor) { myDescriptors.push_back(aDescriptor); }

	size_t GetShaderCount() const { return myShaders.size(); }
	const std::string& GetShader(size_t anInd) const { return myShaders[anInd]; }

private:
	void Serialize(Serializer& aSerializer) override;

	IPipeline::Type myType;
	std::vector<Handle<Descriptor>> myDescriptors;
	std::vector<std::string> myShaders;
};