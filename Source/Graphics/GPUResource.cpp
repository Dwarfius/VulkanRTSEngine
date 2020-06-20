#include "Precomp.h"
#include "GPUResource.h"

#include "Graphics.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"

void GPUResource::Destroy(GPUResource* aResource)
{
	// we don't delete the resource here, instead Graphics will take care of it
	aResource->Unload();
}

GPUResource::GPUResource()
	: myResId(Resource::InvalidId)
	, myState(State::Invalid)
	, myGraphics(nullptr)
	, myKeepResHandle(false)
{
}

void GPUResource::Create(Graphics& aGraphics, Handle<Resource> aRes, bool aShouldKeepRes /* = false*/)
{
	myResHandle = aRes;
	myKeepResHandle = aShouldKeepRes;
	myGraphics = &aGraphics;

	// request dependencies first
	if (myResHandle.IsValid())
	{
		myResId = myResHandle->GetId();
		myResHandle->ExecLambdaOnLoad([=](const Resource* aRes) { UpdateDependencies(aRes); });
	}

	// schedule ourselves for creation
	myState = State::PendingCreate;
	myGraphics->ScheduleCreate(this);
}

void GPUResource::UpdateRegion(UploadRegion aRegion)
{
	ASSERT_STR(myResHandle.IsValid() || myResId == Resource::InvalidId, 
		"Can't update the GPU resource - source resouce has been discarded!");
	myRegionsToUpload.push_back(aRegion);
	myState = State::PendingUpload;
	myGraphics->ScheduleUpload(this);
}

void GPUResource::UpdateRegions(const UploadRegion* aRegions, uint8_t aRegCount)
{
	ASSERT_STR(myResHandle.IsValid() || myResId == Resource::InvalidId,
		"Can't update the GPU resource - source resouce has been discarded!");
	myRegionsToUpload.insert(myRegionsToUpload.end(), aRegions, aRegions + aRegCount);
	myState = State::PendingUpload;
	myGraphics->ScheduleUpload(this);
}

void GPUResource::Unload()
{
	myState = State::PendingUnload;
	myGraphics->ScheduleUnload(this);
}

bool GPUResource::AreDependenciesValid() const
{
	// Not thread safe, but worst case scenario is
	// it's going to be 1 frame delayed result
	for (Handle<GPUResource> dependency : myDependencies)
	{
		if (dependency->GetState() != State::Valid)
		{
			return false;
		}
	}
	return true;
}

void GPUResource::SetErrMsg(std::string&& anErrString)
{
	myState = State::Error;
#ifdef _DEBUG
	myErrorMsg = std::move(anErrString);
	if (myResHandle.IsValid())
	{
		printf("[Error] %s: %s\n", myResHandle->GetPath().c_str(), myErrorMsg.c_str());
	}
	else
	{
		printf("[Error] %s\n", myErrorMsg.c_str());
	}
#endif
}

void GPUResource::TriggerCreate()
{
	OnCreate(*myGraphics);
	UpdateRegion({ 0, 0 });
}

void GPUResource::TriggerUpload()
{
	bool success = OnUpload(*myGraphics);
	myRegionsToUpload.clear();
	if (!myKeepResHandle)
	{
		myResHandle = nullptr;
	}
	if (success)
	{
		myState = State::Valid;
	}
}

void GPUResource::TriggerUnload()
{
	OnUnload(*myGraphics);
	myState = State::Invalid;
}

void GPUResource::UpdateDependencies(const Resource* aRes)
{
	const std::vector<Handle<Resource>>& deps = aRes->GetDependencies();
	for (Handle<Resource> dependency : deps)
	{
		// TODO: using dynamic type checking like this doesn't seem nice:
		//	In theory, types know (or at least, should know) what
		//	types of data they can have dependencies of, meaning this can
		//	all be statically determined. But, writing the machinery for it
		//	feels like overkill for the current 4 types. Also, I don't know
		//	how to handle potential ordering issues, for now.
		Handle<GPUResource> gpuResource;
		Resource* depRes = dependency.Get();
		if (Model* model = dynamic_cast<Model*>(depRes))
		{
			Handle<Model> modelHandle(model);
			gpuResource = myGraphics->GetOrCreate(modelHandle);
		}
		else if (Pipeline* pipeline = dynamic_cast<Pipeline*>(depRes))
		{
			Handle<Pipeline> pipelineHandle(pipeline);
			gpuResource = myGraphics->GetOrCreate(pipelineHandle);
		}
		else if (Shader* shader = dynamic_cast<Shader*>(depRes))
		{
			Handle<Shader> shaderHandle(shader);
			gpuResource = myGraphics->GetOrCreate(shaderHandle);
		}
		else if (Texture* texture = dynamic_cast<Texture*>(depRes))
		{
			Handle<Texture> textureHandle(texture);
			gpuResource = myGraphics->GetOrCreate(textureHandle);
		}
		else
		{
			ASSERT_STR(false, "Unrecognzied type detected!");
		}
		myDependencies.push_back(gpuResource);
	}
}