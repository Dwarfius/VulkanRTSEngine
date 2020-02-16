#include "Precomp.h"
#include "Graphics.h"

#include "Camera.h"
#include "GPUResource.h"

Graphics* Graphics::ourActiveGraphics = NULL;
bool Graphics::ourUseWireframe = false;
int Graphics::ourWidth = 800;
int Graphics::ourHeight = 600;

Graphics::Graphics(AssetTracker& anAssetTracker)
	: myAssetTracker(anAssetTracker)
	, myRenderCalls(0)
	, myWindow(nullptr)
{
}

void Graphics::BeginGather()
{
	ProcessGPUQueues();

	myRenderCalls = 0;

	for (IRenderPass* pass : myRenderPasses)
	{
		pass->BeginPass(*this);
	}
}

void Graphics::Render(IRenderPass::Category aCategory, const Camera& aCam, const RenderJob& aJob, const IRenderPass::IParams& aParams)
{
	// TODO: find a better way to match it
	// find the renderpass fitting the category
	IRenderPass* renderPassToUse = nullptr;
	for (IRenderPass* pass : myRenderPasses)
	{
		if (pass->GetCategory() == aCategory)
		{
			renderPassToUse = pass;
			break;
		}
	}

	renderPassToUse->AddRenderable(aJob, aParams);

	myRenderCalls++;
}

void Graphics::Display()
{
	// trigger submission of all the jobs
	for (IRenderPass* pass : myRenderPasses)
	{
		pass->SubmitJobs(*this);
	}
}

void Graphics::AddRenderPass(IRenderPass* aRenderPass)
{
	myRenderPasses.push_back(aRenderPass);
}

Handle<GPUResource> Graphics::GetOrCreate(Handle<Resource> aRes, bool aShouldKeepRes /*= false*/)
{
	GPUResource* newGPURes;
	const Resource::Id resId = aRes->GetId();
	
	{
		tbb::spin_mutex::scoped_lock lock(myResourceMutex);
		ResourceMap::iterator foundResIter = myResources.find(resId);
		if (foundResIter != myResources.end())
		{
			return foundResIter->second;
		}
		else
		{
			newGPURes = Create(aRes->GetResType());
			myResources[resId] = newGPURes;
		}
	}

	newGPURes->Create(*this, aRes, aShouldKeepRes);
	return newGPURes;
}

void Graphics::ScheduleCreate(Handle<GPUResource> aGPUResource)
{
	ASSERT_STR(aGPUResource->GetState() == GPUResource::State::PendingCreate,
		"Invalid GPU resource state!");
	myCreateQueue.push(aGPUResource);
}

void Graphics::ScheduleUpload(Handle<GPUResource> aGPUResource)
{
	ASSERT_STR(aGPUResource->GetState() == GPUResource::State::PendingUpload,
		"Invalid GPU resource state!");
	ASSERT_STR(!aGPUResource.IsLastHandle(), "Resource will die after being processed!");
	myUploadQueue.push(aGPUResource);
}

void Graphics::ScheduleUnload(GPUResource* aGPUResource)
{
	ASSERT_STR(aGPUResource->GetState() == GPUResource::State::PendingUnload,
		"Invalid GPU resource state!");
	myUnloadQueue.push(aGPUResource);
}

void Graphics::TriggerUpload(GPUResource* aGPUResource)
{
	aGPUResource->TriggerUpload();
}

void Graphics::UnloadAll()
{
	for (ResourceMap::iterator iter = myResources.begin();
		iter != myResources.end();
		iter++)
	{
		iter->second->TriggerUnload();
		delete iter->second;
	}
	myResources.clear();
}

void Graphics::ProcessGPUQueues()
{
	// TODO: overwrite implementation for Vulkan to allow for parallel
	// generation/processing
	Handle<GPUResource> aResourceHandle;
	while(myCreateQueue.try_pop(aResourceHandle))
	{
		if (aResourceHandle.IsLastHandle())
		{
			// last handle - owner discarded, can skip
			// it will automatically get added to the unload queue
			continue;
		}
		aResourceHandle->TriggerCreate();
	}

	std::queue<Handle<GPUResource>> delayQueue;
	while (myUploadQueue.try_pop(aResourceHandle))
	{
		if (aResourceHandle.IsLastHandle())
		{
			// last handle - owner discarded, can skip
			// it will automatically get added to the unload queue
			continue;
		}
		if (!aResourceHandle->AreDependenciesValid())
		{
			// we guarantee that all dependencies will be uploaded
			// before calling the current resource, so delay it
			// to next frame
			delayQueue.push(aResourceHandle);
			continue;
		}
		aResourceHandle->TriggerUpload();
	}
	// reschedule delayed resources from this frame for next
	while (!delayQueue.empty())
	{
		myUploadQueue.push(delayQueue.front());
		delayQueue.pop();
	}

	GPUResource* aResource;
	while (myUnloadQueue.try_pop(aResource))
	{
		aResource->TriggerUnload();
		if (aResource->myResId != Resource::InvalidId)
		{
			myResources.erase(aResource->myResId);
		}
		delete aResource;
	}
}