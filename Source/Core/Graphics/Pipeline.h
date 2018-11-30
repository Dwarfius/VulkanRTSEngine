#pragma once

#include "Resource.h"
#include "Descriptor.h"
#include "Shader.h"

// A base class describing a generic pipeline
class Pipeline : public Resource
{
public:
	enum class Type
	{
		Graphics,
		Compute
	};

	struct CreateDescriptor
	{
	};

	struct UploadDescriptor
	{
		const GPUResource** myShaders;
		size_t myShaderCount;
		const Descriptor* myDescriptors;
		size_t myDescriptorCount;
	};

public:
	Pipeline(Resource::Id anId);
	Pipeline(Resource::Id anId, const string& aPath);

	Resource::Type GetResType() const override { return Resource::Type::Pipeline; }

	size_t GetDescriptorCount() const { return myDescriptors.size(); }
	const Descriptor& GetDescriptor(size_t anIndex) const { return myDescriptors[anIndex]; }

	// takes ownership of the shader
	// call SetState(Resource::State::PendingUpload) when done adding shaders!
	void AddShader(Handle<Shader> aShader);

	void AddDescriptor(Descriptor&& aDescriptor) { myDescriptors.push_back(move(aDescriptor)); }

	size_t GetShaderCount() const { return myShaders.size(); }
	Handle<Shader> GetShader(size_t anInd) const { return myShaders[anInd]; }

private:
	void OnLoad(AssetTracker& anAssetTracker) override;
	void OnUpload(GPUResource* aGPURes) override;

	Type myType;
	vector<Descriptor> myDescriptors;
	vector<Handle<Shader>> myShaders;
};