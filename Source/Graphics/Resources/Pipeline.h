#pragma once

#include "../Resource.h"
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

	Resource::Type GetResType() const override { return Resource::Type::Pipeline; }

	size_t GetDescriptorCount() const override final { return myDescriptors.size(); }
	const Descriptor& GetDescriptor(size_t anIndex) const { return myDescriptors[anIndex]; }

	// takes ownership of the shader
	// call SetState(Resource::State::PendingUpload) when done adding shaders!
	void AddShader(Handle<Shader> aShader);

	void AddDescriptor(Descriptor&& aDescriptor) { myDescriptors.push_back(std::move(aDescriptor)); }

	size_t GetShaderCount() const { return myShaders.size(); }
	Handle<Shader> GetShader(size_t anInd) const { return myShaders[anInd]; }

private:
	void OnLoad(AssetTracker& anAssetTracker, const File& aFile) override;

	IPipeline::Type myType;
	std::vector<Descriptor> myDescriptors;
	std::vector<Handle<Shader>> myShaders;
};