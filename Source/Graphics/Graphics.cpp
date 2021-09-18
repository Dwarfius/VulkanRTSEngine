#include "Precomp.h"
#include "Graphics.h"

#include "Camera.h"
#include "GPUResource.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Resources/GPUModel.h"

#include <Core/Profiler.h>

Graphics::Graphics(AssetTracker& anAssetTracker)
	: myAssetTracker(anAssetTracker)
{
}

void Graphics::Init()
{
	struct PosUVVertex
	{
		glm::vec2 myPos;
		glm::vec2 myUV;

		PosUVVertex() = default;
		constexpr PosUVVertex(glm::vec2 aPos, glm::vec2 aUV)
			: myPos(aPos)
			, myUV(aUV)
		{
		}

		static constexpr VertexDescriptor GetDescriptor()
		{
			using ThisType = PosUVVertex; // for copy-paste convenience
			return {
				sizeof(ThisType),
				2,
				{
					{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myPos) },
					{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myUV) }
				}
			};
		}
	};

	PosUVVertex vertices[] = {
		{ { -1.f,  1.f }, { 0.f, 1.f } },
		{ { -1.f, -1.f }, { 0.f, 0.f } },
		{ {  1.f, -1.f }, { 1.f, 0.f } },

		{ { -1.f,  1.f }, { 0.f, 1.f } },
		{ {  1.f, -1.f }, { 1.f, 0.f } },
		{ {  1.f,  1.f }, { 1.f, 1.f } }
	};
	Model::VertStorage<PosUVVertex>* buffer = new Model::VertStorage<PosUVVertex>(6, vertices);
	Handle<Model> cpuModel = new Model(Model::PrimitiveType::Triangles, buffer, false);
	myFullScrenQuad = GetOrCreate(cpuModel).Get<GPUModel>();
}

void Graphics::BeginGather()
{
	Profiler::ScopedMark profile("Graphics::BeginGather");

	if(myRenderPassesNeedOrdering)
	{
		SortRenderPasses();
		myRenderPassesNeedOrdering = false;
	}

	for (IRenderPass* pass : myRenderPasses)
	{
		pass->BeginPass(*this);
	}
}

void Graphics::Display()
{
	Profiler::ScopedMark profile("Graphics::Display");
	for (IRenderPass* pass : myRenderPasses)
	{
		pass->SubmitJobs(*this);
	}

	// doing it after the submit jobs, because by this time
	// all RenderPassJobs will be created
	if (myRenderPassJobsNeedsOrdering)
	{
		SortRenderPassJobs();
		myRenderPassJobsNeedsOrdering = false;
	}

	// Doing it after SubmitJobs because RenderPasses can 
	// generate new asset updates at any point of their
	// execution
	ProcessGPUQueues();
}

void Graphics::CleanUp()
{
	// we need to delete render passes early as they keep
	// Handles to resources
	for (IRenderPass* pass : myRenderPasses)
	{
		delete pass;
	}

	myFullScrenQuad = Handle<GPUModel>();
}

void Graphics::AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBuffer)
{
	ASSERT_STR(myNamedFrameBuffers.find(aName) == myNamedFrameBuffers.end(),
		"FrameBuffer named %s is already registered!", aName.data());
	myNamedFrameBuffers.insert({ aName, aBuffer });
}

const FrameBuffer& Graphics::GetNamedFrameBuffer(std::string_view aName) const
{
	auto iter = myNamedFrameBuffers.find(aName);
	ASSERT_STR(iter != myNamedFrameBuffers.end(), 
		"Couldn't find a FrameBuffer named %s", aName.data());
	return iter->second;
}

void Graphics::AddRenderPass(IRenderPass* aRenderPass)
{
	// TODO: this is unsafe if done mid frames
	myRenderPasses.push_back(aRenderPass);
	myRenderPassJobsNeedsOrdering = true;
	myRenderPassesNeedOrdering = true;
}

void Graphics::AddRenderPassDependency(RenderPass::Id aPassId, RenderPass::Id aNewDependency)
{
	GetRenderPass(aPassId)->AddDependency(aNewDependency);
	myRenderPassJobsNeedsOrdering = true;
	myRenderPassesNeedOrdering = true;
}

template<class T>
Handle<GPUResource> Graphics::GetOrCreate(Handle<T> aRes, 
	bool aShouldKeepRes /*= false*/,
	GPUResource::UsageType aUsageType /*= GPUResource::UsageType::Static*/)
{
	GPUResource* newGPURes;
	const Resource::Id resId = aRes->GetId();
	if (resId == Resource::InvalidId)
	{
		// invalid id -> dynamic resource, so no need to track it 
		newGPURes = Create(aRes.Get(), aUsageType);
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
			newGPURes = Create(aRes.Get(), aUsageType);
			myResources[resId] = newGPURes;
		}
	}

	newGPURes->Create(*this, aRes, aShouldKeepRes);
	return newGPURes;
}

template Handle<GPUResource> Graphics::GetOrCreate(Handle<Model>, bool, GPUResource::UsageType);
template Handle<GPUResource> Graphics::GetOrCreate(Handle<Pipeline>, bool, GPUResource::UsageType);
template Handle<GPUResource> Graphics::GetOrCreate(Handle<Shader>, bool, GPUResource::UsageType);
template Handle<GPUResource> Graphics::GetOrCreate(Handle<Texture>, bool, GPUResource::UsageType);

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

IRenderPass* Graphics::GetRenderPass(uint32_t anId) const
{
	for (IRenderPass* pass : myRenderPasses)
	{
		if (pass->GetId() == anId)
		{
			return pass;
		}
	}
	ASSERT_STR(false, "Failed to find a render pass for id %d!", anId);
	return nullptr;
}

void Graphics::OnResize(int aWidth, int aHeight)
{
	myWidth = aWidth;
	myHeight = aHeight;
}

void Graphics::SortRenderPasses()
{
	std::sort(myRenderPasses.begin(), myRenderPasses.end(),
		[](const IRenderPass* aLeft, const IRenderPass* aRight) {
		for (const uint32_t rightId : aRight->GetDependencies())
		{
			if (aLeft->GetId() == rightId)
			{
				return true;
			}
		}
		return false;
	});
}