#include "Precomp.h"
#include "Graphics.h"

#include "Camera.h"
#include "GPUResource.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"

#include <Core/Profiler.h>

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
	Profiler::ScopedMark profile("Graphics::BeginGather");
	myRenderCalls = 0;

	for (IRenderPass* pass : myRenderPasses)
	{
		pass->BeginPass(*this);
	}

	// Doing it after BeginPass because they can generate new
	// asset updates
	ProcessGPUQueues();
}

bool Graphics::CanRender(IRenderPass::Category aCategory, const RenderJob& aJob) const
{
	IRenderPass* passToUse = GetRenderPass(aCategory);
	return passToUse->HasResources(aJob);
}

void Graphics::Render(IRenderPass::Category aCategory, RenderJob& aJob, const IRenderPass::IParams& aParams)
{
	IRenderPass* passToUse = GetRenderPass(aCategory);

	passToUse->AddRenderable(aJob, aParams);

	myRenderCalls++;
}

void Graphics::Display()
{
	Profiler::ScopedMark profile("Graphics::Display");
	// trigger submission of all the jobs
	for (IRenderPass* pass : myRenderPasses)
	{
		pass->SubmitJobs(*this);
	}
}

void Graphics::CleanUp()
{
	// we need to delete render passes early as they keep
	// Handles to resources
	for (IRenderPass* pass : myRenderPasses)
	{
		delete pass;
	}
}

void Graphics::AddNamedFrameBuffer(const std::string& aName, const FrameBuffer& aBuffer)
{
	ASSERT_STR(myNamedFrameBuffers.find(aName) == myNamedFrameBuffers.end(),
		"FrameBuffer named %s is already registered!", aName.c_str());
	myNamedFrameBuffers.insert({ aName, aBuffer });
}

const FrameBuffer& Graphics::GetNamedFrameBuffer(const std::string& aName) const
{
	auto iter = myNamedFrameBuffers.find(aName);
	ASSERT_STR(iter != myNamedFrameBuffers.end(), 
		"Couldn't find a FrameBuffer named %s", aName.c_str());
	return iter->second;
}

void Graphics::AddRenderPass(IRenderPass* aRenderPass)
{
	// TODO: this is unsafe if done mid frames
	myRenderPasses.push_back(aRenderPass);
}

template<class T>
Handle<GPUResource> Graphics::GetOrCreate(Handle<T> aRes, bool aShouldKeepRes /*= false*/)
{
	GPUResource* newGPURes;
	const Resource::Id resId = aRes->GetId();
	if (resId == Resource::InvalidId)
	{
		// invalid id -> dynamic resource, so no need to track it 
		newGPURes = Create(aRes.Get());
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
			newGPURes = Create(aRes.Get());
			myResources[resId] = newGPURes;
		}
	}

	newGPURes->Create(*this, aRes, aShouldKeepRes);
	return newGPURes;
}

template Handle<GPUResource> Graphics::GetOrCreate(Handle<Model>, bool);
template Handle<GPUResource> Graphics::GetOrCreate(Handle<Pipeline>, bool);
template Handle<GPUResource> Graphics::GetOrCreate(Handle<Shader>, bool);
template Handle<GPUResource> Graphics::GetOrCreate(Handle<Texture>, bool);

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

void Graphics::ProcessUnloadQueue()
{
	Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::Unload");
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

void Graphics::ProcessGPUQueues()
{
	// TODO: overwrite implementation for Vulkan to allow for parallel
	// generation/processing
	Handle<GPUResource> aResourceHandle;
	std::queue<Handle<GPUResource>> delayQueue;

	{
		Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::Create");
		while (myCreateQueue.try_pop(aResourceHandle))
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
	}

	{
		Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::Upload");
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
	}

	ProcessUnloadQueue();
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