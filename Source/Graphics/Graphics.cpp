#include "Precomp.h"
#include "Graphics.h"

#include "Camera.h"
#include "GPUResource.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Resources/GPUModel.h"
#include "Resources/UniformBuffer.h"

#include <Core/Profiler.h>

Graphics::Graphics(AssetTracker& anAssetTracker)
	: myAssetTracker(anAssetTracker)
{
	// we will preserve resources until they can be used,
	// need to move active writing queue to 1
	// before the kMaxFramesScheduled
	for (uint8_t i = 0; i < GraphicsConfig::kMaxFramesScheduled - 1; i++)
	{
		myUnloadQueues.AdvanceWrite();
	}
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

	myUnloadQueues.AdvanceWrite();

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

void Graphics::EndGather()
{
	Profiler::ScopedMark profile("Graphics::EndGather");
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
}

void Graphics::Display()
{
	Profiler::ScopedMark profile("Graphics::Display");
	// Doing it after EndGather because RenderPasses can 
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
	Resource::Id resId = aRes->GetId();
	if (resId == Resource::InvalidId)
	{
		// dynamic resource, so generate an Id to associate and track it
		myAssetTracker.AssignDynamicId(*aRes.Get());
		resId = aRes->GetId();
		newGPURes = Create(aRes.Get(), aUsageType);
		{
			tbb::spin_mutex::scoped_lock lock(myResourceMutex);
			myResources[resId] = newGPURes;
		}
	}
	else
	{
		tbb::spin_mutex::scoped_lock lock(myResourceMutex);
		ResourceMap::iterator foundResIter = myResources.find(resId);
		if (foundResIter != myResources.end()
			// If a resource got scheduled for unloading - 
			// just create a new one instead
			&& foundResIter->second->GetState() != GPUResource::State::PendingUnload)
		{
			// In theory, we can still return a resource that
			// has just been scheduled for uploading, but I'm 
			// not sure it'll happe. I might need to make 
			// GPUResource's state atomic, as currently I rely on
			// above spin-lock to ensure it'll be available across
			// threads
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
	UnregisterResource(aGPUResource);
	myUnloadQueues.GetWrite().push(aGPUResource);
}

Handle<UniformBuffer> Graphics::CreateUniformBuffer(size_t aSize)
{
	Handle<UniformBuffer> uniformBuffer = CreateUniformBufferImpl(aSize);
	uniformBuffer->Create(*this, {});
	return uniformBuffer;
}

void Graphics::TriggerUpload(GPUResource* aGPUResource)
{
	aGPUResource->TriggerUpload();
}

void Graphics::TriggerUnload(GPUResource* aGPUResource)
{
	aGPUResource->TriggerUnload();
}

void Graphics::ProcessCreateQueue()
{
	Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::Create");
	std::queue<Handle<GPUResource>> delayQueue;
	Handle<GPUResource> aResourceHandle;
	while (myCreateQueue.try_pop(aResourceHandle))
	{
		if (aResourceHandle.IsLastHandle())
		{
			// last handle - owner discarded, so can unregister
			{
				tbb::spin_mutex::scoped_lock lock(myResourceMutex);
				myResources.erase(aResourceHandle->myResId);
			}
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

void Graphics::ProcessUploadQueue()
{
	Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::Upload");
	std::queue<Handle<GPUResource>> delayQueue;
	Handle<GPUResource> aResourceHandle;
	while (myUploadQueue.try_pop(aResourceHandle))
	{
		if (aResourceHandle.IsLastHandle())
		{
			// last handle - owner discarded, so can unregister
			{
				tbb::spin_mutex::scoped_lock lock(myResourceMutex);
				myResources.erase(aResourceHandle->myResId);
			}
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

void Graphics::ProcessNextUnloadQueue()
{
	Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::Unload");
	myUnloadQueues.AdvanceRead();
	GPUResource* aResource;
	tbb::concurrent_queue<GPUResource*>& unloadQueue = myUnloadQueues.GetRead();
	while (unloadQueue.try_pop(aResource))
	{
		DEBUG_ONLY(
			Handle<GPUResource> tempHandle(aResource);
			ASSERT_STR(tempHandle.IsLastHandle(), 
				"Trying to cleanup something still in use!"
			);
		);
		aResource->TriggerUnload();
		{
			tbb::spin_mutex::scoped_lock lock(myResourceMutex);
			myResources.erase(aResource->myResId);
		}
		delete aResource;
	}
}

void Graphics::ProcessAllUnloadQueues()
{
	Profiler::ScopedMark profile("Graphics::ProcessGPUQueues::UnloadAll");
	for (tbb::concurrent_queue<GPUResource*>& unloadQueue : myUnloadQueues)
	{
		GPUResource* aResource;
		while (unloadQueue.try_pop(aResource))
		{
			aResource->TriggerUnload();
			{
				tbb::spin_mutex::scoped_lock lock(myResourceMutex);
				myResources.erase(aResource->myResId);
			}
			delete aResource;
		}
	}
}

void Graphics::UnregisterResource(GPUResource* aRes)
{
	ASSERT_STR(aRes->GetState() == GPUResource::State::PendingUnload,
		"Resource must be at end of it's life for it to be unregistered!");
	tbb::spin_mutex::scoped_lock lock(myResourceMutex);
	myResources.erase(aRes->myResId);
}

void Graphics::ProcessGPUQueues()
{
	ProcessCreateQueue();
	ProcessUploadQueue();
	ProcessNextUnloadQueue();
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