#include "Precomp.h"
#include "GPUResource.h"

#include "Graphics.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"

static_assert(std::is_same_v<GPUResource::ResourceId, Resource::Id>, "Update to match!");

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
}

void GPUResource::Create(Graphics& aGraphics, Handle<Resource> aRes, bool aShouldKeepRes /* = false*/)
{
	myResHandle = aRes;
	myKeepResHandle = aShouldKeepRes;
	myGraphics = &aGraphics;

	if (myResHandle.IsValid())
	{
		// Remember resource ID so we can track GPUResource=>Resource
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
	myGraphics->ScheduleUpload(this);
}

void GPUResource::UpdateRegions(const UploadRegion* aRegions, uint8_t aRegCount)
{
	ASSERT_STR(myResHandle.IsValid() || myResId == Resource::InvalidId,
		"Can't update the GPU resource - source resource has been discarded!");
	myRegionsToUpload.insert(myRegionsToUpload.end(), aRegions, aRegions + aRegCount);
	myGraphics->ScheduleUpload(this);
}

void GPUResource::Unload()
{
	myState = State::PendingUnload;
	myGraphics->ScheduleUnload(this);
}

bool GPUResource::AreDependenciesValid() const
{
	// Hot-reload can schedule uploading before the resource has finished loading.
	// In that case, we are not ready
	if (myResHandle.IsValid() && myResHandle->GetState() != Resource::State::Ready)
	{
		return false;
	}

	return true;
}

void GPUResource::AddDependent(GPUResource* aDependent)
{
#if ASSERT_MUTEX
	AssertWriteLock lock(myDependentsMutex);
#endif
	ASSERT_STR(!std::ranges::contains(myDependents, aDependent),
		"Dependent is already tracked!");
	myDependents.push_back(aDependent);
}

void GPUResource::RemoveDependent(GPUResource* aDependent)
{
#if ASSERT_MUTEX
	AssertWriteLock lock(myDependentsMutex);
#endif
	std::erase(myDependents, aDependent);
}

void GPUResource::SetErrMsg(std::string_view anErrString)
{
	myState = State::Error;
#ifdef _DEBUG
	if (myResHandle.IsValid())
	{
		std::println("[Error] {}: {}", myResHandle->GetPath(), anErrString);
	}
	else
	{
		std::println("[Error] {}", anErrString);
	}
#endif
}

void GPUResource::TriggerCreate()
{
	OnCreate(*myGraphics);
	myState = State::PendingUpload;
	UpdateRegion({ 0, 0 });
}

bool GPUResource::TriggerUpload()
{
	const bool success = OnUpload(*myGraphics);
	if (success)
	{
		myRegionsToUpload.clear();
		if (!myKeepResHandle)
		{
			myResHandle = nullptr;
		}
		myState = State::Valid;
	}
	return success;
}

void GPUResource::TriggerUnload()
{
	OnUnload(*myGraphics);
	myState = State::Invalid;
}