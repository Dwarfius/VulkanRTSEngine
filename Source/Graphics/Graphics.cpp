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

bool Graphics::CanRender(IRenderPass::Category aCategory, const RenderJob& aJob) const
{
	IRenderPass* passToUse = GetRenderPass(aCategory);
	return passToUse->HasResources(aJob);
}

void Graphics::Render(IRenderPass::Category aCategory, const RenderJob& aJob, const IRenderPass::IParams& aParams)
{
	IRenderPass* passToUse = GetRenderPass(aCategory);

	passToUse->AddRenderable(aJob, aParams);

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
	if (resId == Resource::InvalidId)
	{
		// invalid id -> dynamic resource, so no need to track it 
		newGPURes = Create(aRes->GetResType());
	}
	else
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
	std::queue<Handle<GPUResource>> delayQueue;
	while(myCreateQueue.try_pop(aResourceHandle))
	{
		if (aResourceHandle.IsLastHandle())
		{
			// last handle - owner discarded, can skip
			// it will automatically get added to the unload queue
			continue;
		}
		if (aResourceHandle->GetResource().IsValid()
			&& aResourceHandle->GetResource()->GetState() != Resource::State::Ready)
		{
			// we want to guarantee that the source resource has finished
			// loading up to enable correct setup of gpu resource
			delayQueue.push(aResourceHandle);
			continue;
		}
		aResourceHandle->TriggerCreate();
	}
	// reschedule delayed resources from this frame for next
	while (!delayQueue.empty())
	{
		myCreateQueue.push(delayQueue.front());
		delayQueue.pop();
	}

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

IRenderPass* Graphics::GetRenderPass(IRenderPass::Category aCategory) const
{
	// TODO: find a better way to match it - through enum ind
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
	ASSERT_STR(renderPassToUse, "Failed to find a render pass for id %d!", static_cast<uint32_t>(aCategory));
	return renderPassToUse;
}