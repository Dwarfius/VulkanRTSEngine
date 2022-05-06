#include "Precomp.h"
#include "GPUResource.h"

#include "Graphics.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"

GPUResource::GPUResource()
	: myResId(Resource::InvalidId)
	, myState(State::Invalid)
	, myGraphics(nullptr)
	, myKeepResHandle(false)
{
}

void GPUResource::Cleanup()
{
	if (GetState() == GPUResource::State::Valid)
	{
		// we don't delete the resource here, instead Graphics will take care of it
		Unload();
	}
	// we can also immediately drop our dependencies
	// this schedules their removal earlier
	myDependencies.clear();
}

void GPUResource::Create(Graphics& aGraphics, Handle<Resource> aRes, bool aShouldKeepRes /* = false*/)
{
	myResHandle = aRes;
	myKeepResHandle = aShouldKeepRes;
	myGraphics = &aGraphics;

	if (myResHandle.IsValid())
	{
		// Remember ressource ID so we can track GPUResource=>Resource
		myResId = myResHandle->GetId();
	}

	// schedule ourselves for creation
	myState = State::PendingCreate;
	myGraphics->ScheduleCreate(this);
}

void GPUResource::UpdateRegion(UploadRegion aRegion)
{
	ASSERT_STR(myResHandle.IsValid() || myResId == Resource::InvalidId, 
		"Can't update the GPU resource - source resource has been discarded!");
	myRegionsToUpload.push_back(aRegion);
	myState = State::PendingUpload;
	myGraphics->ScheduleUpload(this);
}

void GPUResource::UpdateRegions(const UploadRegion* aRegions, uint8_t aRegCount)
{
	ASSERT_STR(myResHandle.IsValid() || myResId == Resource::InvalidId,
		"Can't update the GPU resource - source resource has been discarded!");
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

void GPUResource::SetErrMsg(std::string_view anErrString)
{
	myState = State::Error;
#ifdef _DEBUG
	if (myResHandle.IsValid())
	{
		printf("[Error] %s: %s\n", myResHandle->GetPath().c_str(), anErrString.data());
	}
	else
	{
		printf("[Error] %s\n", anErrString.data());
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