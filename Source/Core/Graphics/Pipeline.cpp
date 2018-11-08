#include "Precomp.h"
#include "Pipeline.h"

#include "Graphics.h"

Pipeline::Pipeline(Resource::Id anId)
	: Pipeline(anId, "")
{
}

Pipeline::Pipeline(Resource::Id anId, const string& aPath)
	: Resource(anId, aPath)
	, myType(Type::Graphics)
	, myDescriptor("UniformAdapter") // HACK!
{
	// HACK as well!
	myDescriptor.SetUniformType(0, UniformType::Mat4);
	myDescriptor.SetUniformType(1, UniformType::Mat4);
	myDescriptor.RecomputeSize();
}

void Pipeline::AddShader(Handle<Shader> aShader)
{
	ASSERT_STR(myType == Type::Compute && aShader->GetType() == Shader::Type::Compute
		|| myType == Type::Graphics && aShader->GetType() != Shader::Type::Compute,
		"Attempted to pass an incompatible shader to the pipeline!");
	myShaders.push_back(aShader);
	myDependencies.push_back(aShader.Get<Resource>());
}

void Pipeline::Load()
{
	// TODO: read the configuration .ppl file that will pick up on what's needed for proper
	// work of the pipeline - what type of pipeline, what shaders it uses for each stage, etc.
	// For now we're just updating our state so that can work as usual
	myState = State::PendingUpload;
}

void Pipeline::Upload(GPUResource* aGPURes)
{
	ASSERT_STR(myShaders.size() > 0, "Attempted to upload a pipeline with no shaders attached!");

	myGPUResource = aGPURes;

	CreateDescriptor createDesc;
	myGPUResource->Create(createDesc);

	vector<const GPUResource*> shaders;
	shaders.resize(myShaders.size());
	for (size_t i=0; i<myShaders.size(); i++)
	{
		shaders[i] = &myShaders[i]->GetGPUResource();
	}
	UploadDescriptor uploadDesc;
	uploadDesc.myShaders = shaders.data();
	uploadDesc.myShaderCount = myShaders.size();
	bool success = myGPUResource->Upload(uploadDesc);

	if (success)
	{
		myState = State::Ready;
	}
	else
	{
		SetErrMsg(myGPUResource->GetErrorMsg());
	}
}