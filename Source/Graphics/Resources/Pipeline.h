#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/IPipeline.h"
#include "../Descriptor.h"
#include "../Resources/Shader.h"

class UniformAdapter;

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
	const UniformAdapter& GetAdapter(size_t anIndex) const override final { return myAdapters[anIndex]; }

	void AddShader(const std::string& aShader) { myShaders.push_back(aShader); }

	void AddDescriptor(Handle<Descriptor> aDescriptor);

	size_t GetShaderCount() const { return myShaders.size(); }
	const std::string& GetShader(size_t anInd) const { return myShaders[anInd]; }

private:
	void Serialize(Serializer& aSerializer) override;

	void AddAdapter(Handle<Descriptor>& aDescriptor);
	void AssignAdapters();

	IPipeline::Type myType;
	// TODO: replace with embedded struct instead of separate Resource!
	std::vector<Handle<Descriptor>> myDescriptors;
	std::vector<std::reference_wrapper<const UniformAdapter>> myAdapters;
	std::vector<std::string> myShaders;
};