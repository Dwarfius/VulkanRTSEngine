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
	};

public:
	Pipeline(Resource::Id anId);
	Pipeline(Resource::Id anId, const string& aPath);

	Resource::Type GetResType() const override { return Resource::Type::Pipeline; }

	const Descriptor& GetDescriptor() const { return myDescriptor; }

	// takes ownership of the shader
	// call SetState(Resource::State::PendingUpload) when done adding shaders!
	void AddShader(Handle<Shader> aShader);

	size_t GetShaderCount() const { return myShaders.size(); }
	Handle<Shader> GetShader(size_t anInd) const { return myShaders[anInd]; }

private:
	void Load() override;
	void Upload(GPUResource* aGPURes) override;

	Type myType;
	Descriptor myDescriptor;
	vector<Handle<Shader>> myShaders;
};