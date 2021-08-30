#pragma once

#include "FrameBuffer.h"
#include "RenderPass.h"
#include "UniformBlock.h"

#include <Core/Resources/Resource.h> // needed for Resource::Id
#include <Graphics/GPUResource.h>

struct GLFWwindow;
class Camera;
class Terrain;
class AssetTracker;
class RenderPassJob;
class RenderContext;
class Texture;
class Pipeline;
class Model;
class Shader;

class Graphics
{
public:
	// TODO: move this to a settings struct
	static bool ourUseWireframe;

public:
	Graphics(AssetTracker& anAssetTracker);
	virtual ~Graphics() = default;

	virtual void Init() = 0;
	virtual void BeginGather();
	virtual void Display();
	virtual void CleanUp();
	
	GLFWwindow* GetWindow() const { return myWindow; }
	
	static float GetWidth() { return static_cast<float>(ourWidth); }
	static float GetHeight() { return static_cast<float>(ourHeight); }

	// Graphics will manage the lifetime of the render pass job,
	// but it guarantees that it will stay valid until the next call
	// to GetRenderPassJob with same context.
	// Threadsafe
	[[nodiscard]] 
	virtual RenderPassJob& GetRenderPassJob(IRenderPass::Id anId, const RenderContext& renderContext) = 0;

	virtual void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer);
	const FrameBuffer& GetNamedFrameBuffer(std::string_view aName) const;

	// Notifies the rendering system about how many threads will access it
	virtual void SetMaxThreads(uint32_t aMaxThreadCount) {}

	// Adds a render pass to graphics
	// takes ownership of the renderpass
	void AddRenderPass(IRenderPass* aRenderPass);
	template<class T>
	T* GetRenderPass()
	{
		return static_cast<T*>(GetRenderPass(T::kId));
	}
	template<class T>
	const T* GetRenderPass() const
	{
		return static_cast<const T*>(GetRenderPass(T::kId));
	}

	template<class T>
	Handle<GPUResource> GetOrCreate(Handle<T> aRes, 
		bool aShouldKeepRes = false, 
		GPUResource::UsageType aUsageType = GPUResource::UsageType::Static
	);
	void ScheduleCreate(Handle<GPUResource> aGPUResource);
	void ScheduleUpload(Handle<GPUResource> aGPUResource);
	void ScheduleUnload(GPUResource* aGPUResource);

	AssetTracker& GetAssetTracker() { return myAssetTracker; }

protected:
	static int ourWidth, ourHeight;

	void TriggerUpload(GPUResource* aGPUResource);

	AssetTracker& myAssetTracker;

	GLFWwindow* myWindow;
	std::vector<IRenderPass*> myRenderPasses;
	std::unordered_map<std::string_view, FrameBuffer> myNamedFrameBuffers;

	void ProcessUnloadQueue();
	bool AreResourcesEmpty() const { return myResources.empty(); }

private:
	using ResourceMap = std::unordered_map<Resource::Id, GPUResource*>;

	tbb::concurrent_queue<Handle<GPUResource>> myCreateQueue;
	tbb::concurrent_queue<Handle<GPUResource>> myUploadQueue;
	// Not using handles since it already made here after last handle
	// got destroyed, or requested directly
	tbb::concurrent_queue<GPUResource*> myUnloadQueue;
	ResourceMap myResources;
	tbb::spin_mutex myResourceMutex;

	void ProcessGPUQueues();

	virtual GPUResource* Create(Model*, GPUResource::UsageType aUsage) const = 0;
	virtual GPUResource* Create(Pipeline*, GPUResource::UsageType aUsage) const = 0;
	virtual GPUResource* Create(Shader*, GPUResource::UsageType aUsage) const = 0;
	virtual GPUResource* Create(Texture*, GPUResource::UsageType aUsage) const = 0;

	IRenderPass* GetRenderPass(uint32_t anId) const;
};