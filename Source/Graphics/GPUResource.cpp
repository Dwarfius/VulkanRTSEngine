#include "Precomp.h"
#include "GPUResource.h"

#include "Graphics.h"

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
	if (myResHandle.IsValid() && myResHandle->GetState() != Resource::State::Ready)
	{
		// if we have a resource, then it must finish loading, 
		// or we can't upload from it
		return false;
	}
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
	printf("[Error] %s\n", myErrorMsg.c_str());
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
		myDependencies.push_back(myGraphics->GetOrCreate(dependency));
	}
}