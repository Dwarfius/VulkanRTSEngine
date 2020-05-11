#pragma once

#include <Core/Vertex.h>
#include "UniformBlock.h"
#include "RenderPass.h"
#include "Resource.h" // needed for Resource::Id

struct GLFWwindow;
class Camera;
class Terrain;
class AssetTracker;
class DebugDrawer;
class RenderPassJob;
class RenderContext;
class GPUResource;

class Graphics
{
public:
	// TODO: move this to a settings struct
	static bool ourUseWireframe;

public:
	Graphics(AssetTracker& anAssetTracker);

	virtual void Init() = 0;
	virtual void BeginGather();
	bool CanRender(IRenderPass::Category aCategory, const RenderJob& aJob) const;
	void Render(IRenderPass::Category aCategory, const RenderJob& aJob, const IRenderPass::IParams& aParams);
	virtual void Display();
	virtual void CleanUp() = 0;
	
	virtual void PrepareLineCache(size_t aCacheSize) = 0;
	virtual void RenderDebug(const Camera& aCam, const DebugDrawer& aDebugDrawer) = 0;

	GLFWwindow* GetWindow() const { return myWindow; }
	
	uint32_t GetRenderCalls() const { return myRenderCalls; }

	static float GetWidth() { return static_cast<float>(ourWidth); }
	static float GetHeight() { return static_cast<float>(ourHeight); }

	// Graphics will manage the lifetime of the render pass job,
	// but it guarantees that it will stay valid until the next call
	// to GetRenderPassJob with same context.
	// Threadsafe
	[[nodiscard]] 
	virtual RenderPassJob& GetRenderPassJob(uint32_t anId, const RenderContext& renderContext) = 0;

	// Notifies the rendering system about how many threads will access it
	virtual void SetMaxThreads(uint32_t aMaxThreadCount) {}

	// Adds a render pass to graphics
	// takes ownership of the renderpass
	void AddRenderPass(IRenderPass* aRenderPass);

	Handle<GPUResource> GetOrCreate(Handle<Resource> aRes, bool aShouldKeepRes = false);
	void ScheduleCreate(Handle<GPUResource> aGPUResource);
	void ScheduleUpload(Handle<GPUResource> aGPUResource);
	void ScheduleUnload(GPUResource* aGPUResource);

	AssetTracker& GetAssetTracker() { return myAssetTracker; }

protected:
	static int ourWidth, ourHeight;
	static Graphics* ourActiveGraphics;

	void TriggerUpload(GPUResource* aGPUResource);

	AssetTracker& myAssetTracker;

	GLFWwindow* myWindow;
	uint32_t myRenderCalls;
	std::vector<IRenderPass*> myRenderPasses;

	void UnloadAll();

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

	virtual GPUResource* Create(Resource::Type aType) const = 0;

	IRenderPass* GetRenderPass(IRenderPass::Category aCategory) const;
};

