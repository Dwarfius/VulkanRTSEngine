#include "Precomp.h"
#include "Graphics.h"

#include "Camera.h"
#include "GPUResource.h"
#include "Resources/Model.h"
#include "Resources/Pipeline.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Resources/GPUPipeline.h"
#include "Resources/GPUModel.h"
#include "Resources/GPUTexture.h"
#include "Resources/GPUShader.h"
#include "Resources/GPUBuffer.h"
#include "UniformAdapterRegister.h"

#include <Core/Profiler.h>

std::vector<const GPUResource*> Graphics::DebugAccess::GetResources(Graphics& aGraphics)
{
	tbb::spin_mutex::scoped_lock lock(aGraphics.myResourceMutex);
	std::vector<const GPUResource*> resources;
	resources.reserve(aGraphics.myResources.size());
	for (const auto& [id, res] : aGraphics.myResources)
	{
		resources.emplace_back(res);
	}
	return resources;
}

std::vector<std::pair<std::string_view, FrameBuffer>> Graphics::DebugAccess::GetFrameBuffers(Graphics& aGraphics)
{
	std::vector<std::pair<std::string_view, FrameBuffer>> frameBuffers;
	frameBuffers.reserve(aGraphics.myNamedFrameBuffers.size());
	for (const auto& [name, buffer] : aGraphics.myNamedFrameBuffers)
	{
		frameBuffers.emplace_back(name, buffer);
	}
	return frameBuffers;
}

std::vector<RenderPass*> Graphics::DebugAccess::GetRenderPasses(Graphics& aGraphics)
{
	return aGraphics.myRenderPasses;
}

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
	Handle<Model> cpuModel = new Model(
		Model::PrimitiveType::Triangles, 
		std::span<PosUVVertex>{vertices},
		false
	);
	myFullScrenQuad = GetOrCreate(cpuModel).Get<GPUModel>();

	ProcessCreateQueue();
}

void Graphics::Gather()
{
	Profiler::ScopedMark profile("Graphics::Gather");

	myUnloadQueues.AdvanceWrite();

	if(myRenderPassesNeedOrdering)
	{
		SortRenderPasses();
		myRenderPassesNeedOrdering = false;
	}

	for (RenderPass* pass : myRenderPasses)
	{
		pass->Execute(*this);
	}
}

void Graphics::Display()
{
	Profiler::ScopedMark profile("Graphics::Display");
	// Doing it after Gather because RenderPasses can 
	// generate new asset updates at any point of their
	// execution
	ProcessGPUQueues();
}

void Graphics::CleanUp()
{
	// we need to delete render passes early as they keep
	// Handles to resources
	for (RenderPass* pass : myRenderPasses)
	{
		delete pass;
	}

	myFullScrenQuad = Handle<GPUModel>();
}

void Graphics::AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBuffer)
{
	ASSERT_STR(myNamedFrameBuffers.find(aName) == myNamedFrameBuffers.end(),
		"FrameBuffer named {} is already registered!", aName.data());
	myNamedFrameBuffers.insert({ aName, aBuffer });
}

const FrameBuffer& Graphics::GetNamedFrameBuffer(std::string_view aName) const
{
	auto iter = myNamedFrameBuffers.find(aName);
	ASSERT_STR(iter != myNamedFrameBuffers.end(), 
		"Couldn't find a FrameBuffer named {}", aName.data());
	return iter->second;
}

void Graphics::AddRenderPass(RenderPass* aRenderPass)
{
	// TODO: this is unsafe if done mid frames
	myRenderPasses.push_back(aRenderPass);
	myRenderPassesNeedOrdering = true;
}

void Graphics::AddRenderPassDependency(RenderPass::Id aPassId, RenderPass::Id aNewDependency)
{
	GetRenderPass(aPassId)->AddDependency(aNewDependency);
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
	ASSERT_STR(aGPUResource->GetState() == GPUResource::State::PendingUpload
		|| aGPUResource->GetState() == GPUResource::State::Valid,
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

Handle<GPUBuffer> Graphics::CreateGPUBuffer(size_t aSize, uint8_t aFrameCount, bool aIsUBO)
{
	Handle<GPUBuffer> uniformBuffer = CreateGPUBufferImpl(aSize, aFrameCount, aIsUBO);
	uniformBuffer->Create(*this, {});
	return uniformBuffer;
}

void Graphics::FileChanged(std::string_view aFile)
{
	// Because we can drop CPU-side resources when we upload
	// GPU-resources, we have to check if we currently have a GPU-resource
	// loaded - otherwise we might end up loading resources that we don't
	// actually need. 
	AssetTracker::ResIdPair resInfo = myAssetTracker.FindRes(aFile);
	if (resInfo.myId == Resource::InvalidId)
	{
		return;
	}

	// Since AssetTracker knows about a resource, it's either one belonging to
	// GPU or belongs to CPU only. 
	Handle<GPUResource> gpuRes;
	{
		tbb::spin_mutex::scoped_lock lock(myResourceMutex);
		auto iter = myResources.find(resInfo.myId);
		if (iter == myResources.end())
		{
			return;
		}
		gpuRes = iter->second;
	}

	if (gpuRes.IsValid())
	{
		// If it is a GPUResource, the underlying Resource might be unloaded
		// so we will need to load it before we can push an update to GPU.
		// That's why we'll force the load
		Handle<Resource> res = myAssetTracker.ResourceChanged(resInfo, true);
		ASSERT(res.IsValid());

		if (gpuRes->GetState() == GPUResource::State::Valid)
		{
			gpuRes->myResHandle = res;
			ScheduleUpload(gpuRes);
		}
	}
	else
	{
		// Otherwise it's a CPU resource, and no GPU operations are necessary,
		// so we will let AssetTracker decide what to do with it
		myAssetTracker.ResourceChanged(resInfo);
	}
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
		const bool isReloaded = aResourceHandle->GetState() == GPUResource::State::Valid;
		const bool uploaded = aResourceHandle->TriggerUpload();
		if (uploaded && isReloaded)
		{
			// If we reloaded succesfully, time to notify all dependents that 
			// they're ready to reload
			aResourceHandle->ForEachDependent([&](GPUResource* aRes) {
				if (aRes->GetState() != GPUResource::State::Valid)
				{
					return;
				}

				AssetTracker::ResIdPair resInfo{ {}, aRes->myResId };
				Handle<Resource> res = myAssetTracker.ResourceChanged(resInfo, true);
				if (res.IsValid())
				{
					aRes->myResHandle = res;
					ScheduleUpload(aRes);
				}
			});
		}
		// If we failed to upload, we can be in 3 states: Valid(deferred, hot reload), 
		// PendingUpload(deferred, first upload) and Error
		else if (!uploaded && aResourceHandle->GetState() != GPUResource::State::Error)
		{
			delayQueue.push(aResourceHandle);
		}
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
			{
				Handle<GPUResource> tempHandle(aResource);
				ASSERT_STR(tempHandle.IsLastHandle(),
					"Trying to cleanup something still in use!"
				);
			}
		);
		aResource->TriggerUnload();
		{
			tbb::spin_mutex::scoped_lock lock(myResourceMutex);
			myResources.erase(aResource->myResId);
		}
		DeleteResource(aResource);
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
			DeleteResource(aResource);
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

void Graphics::DeleteResource(GPUResource* aResource)
{
	delete aResource;
}

void Graphics::ProcessGPUQueues()
{
	ProcessCreateQueue();
	ProcessUploadQueue();
	ProcessNextUnloadQueue();
}

RenderPass* Graphics::GetRenderPass(uint32_t anId) const
{
	for (RenderPass* pass : myRenderPasses)
	{
		if (pass->GetId() == anId)
		{
			return pass;
		}
	}
	ASSERT_STR(false, "Failed to find a render pass for id {}!", anId);
	return nullptr;
}

void Graphics::OnResize(int aWidth, int aHeight)
{
	myWidth = aWidth;
	myHeight = aHeight;
}

void Graphics::SortRenderPasses()
{
	// TODO: take into account frame-buffers
	std::vector<RenderPass*> toSort = std::move(myRenderPasses);
	myRenderPasses.reserve(toSort.size());
	std::unordered_set<uint32_t> alreadyAdded;
	alreadyAdded.reserve(toSort.size());
	uint32_t startIndex = 0;
	while (startIndex < toSort.size())
	{
		bool inserted = false;
		for (uint32_t i = startIndex; i < toSort.size(); i++)
		{
			RenderPass* pass = toSort[i];
			bool foundDeps = true;
			for(uint32_t dep : pass->GetDependencies())
			{
				if (!alreadyAdded.contains(dep))
				{
					foundDeps = false;
					break;
				}
			}
			if (foundDeps)
			{
				alreadyAdded.emplace(pass->GetId());
				myRenderPasses.push_back(pass);
				if (i != startIndex)
				{
					std::swap(toSort[i], toSort[startIndex]);
				}
				startIndex++;
				inserted = true;
				break;
			}
		}
		ASSERT_STR(inserted, "Missing dependency for one of the render passes!");
	}
}