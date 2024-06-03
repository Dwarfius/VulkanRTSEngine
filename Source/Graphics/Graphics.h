#pragma once

#include "FrameBuffer.h"
#include "RenderPass.h"
#include "UniformBlock.h"

#include <Core/Resources/Resource.h> // needed for Resource::Id
#include <Core/RWBuffer.h>
#include <Graphics/GPUResource.h>
#include <Graphics/GraphicsConfig.h>

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
class GPUModel;
class UniformBuffer;

class Graphics
{
public:
	struct DebugAccess
	{
		// Returns a snapshot of all resources
		// Note: not thread safe if called during Graphics queue processing!
		static std::vector<const GPUResource*> GetResources(Graphics& aGraphics);
	};

public:
	Graphics(AssetTracker& anAssetTracker);
	virtual ~Graphics() = default;

	virtual void Init();
	virtual void Gather();
	virtual void Display();
	virtual void CleanUp();
	
	GLFWwindow* GetWindow() const { return myWindow; }
	
	float GetWidth() const { return static_cast<float>(myWidth); }
	float GetHeight() const { return static_cast<float>(myHeight); }

	// Threadsafe
	[[nodiscard]] 
	virtual RenderPassJob& CreateRenderPassJob(const RenderContext& renderContext) = 0;

	virtual void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer);
	const FrameBuffer& GetNamedFrameBuffer(std::string_view aName) const;
	virtual void ResizeNamedFrameBuffer(std::string_view aName, glm::ivec2 aSize) {}

	// Notifies the rendering system about how many threads will access it
	virtual void SetMaxThreads(uint32_t aMaxThreadCount) {}

	// Adds a render pass to graphics
	// takes ownership of the renderpass
	void AddRenderPass(RenderPass* aRenderPass);
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

	void AddRenderPassDependency(RenderPass::Id aPassId, RenderPass::Id aNewDependency);

	template<class T>
	Handle<GPUResource> GetOrCreate(Handle<T> aRes, 
		bool aShouldKeepRes = false, 
		GPUResource::UsageType aUsageType = GPUResource::UsageType::Static
	);
	void ScheduleCreate(Handle<GPUResource> aGPUResource);
	void ScheduleUpload(Handle<GPUResource> aGPUResource);
	void ScheduleUnload(GPUResource* aGPUResource);
	virtual void CleanUpUBO(UniformBuffer* aUBO) = 0;

	Handle<UniformBuffer> CreateUniformBuffer(size_t aSize);

	AssetTracker& GetAssetTracker() { return myAssetTracker; }

	Handle<GPUModel> GetFullScreenQuad() const { return myFullScrenQuad; }

	void FileChanged(std::string_view aFile);

	virtual std::string_view GetTypeName() const = 0;

protected:
	void TriggerUpload(GPUResource* aGPUResource);
	void TriggerUnload(GPUResource* aGPUResource);

	AssetTracker& myAssetTracker;

	GLFWwindow* myWindow = nullptr;
	std::vector<RenderPass*> myRenderPasses;
	std::unordered_map<std::string_view, FrameBuffer> myNamedFrameBuffers;

	void ProcessCreateQueue();
	void ProcessUploadQueue();
	// Unloads resoruces that were scheduled for next frame
	void ProcessNextUnloadQueue();
	// Unloads all resources that were scheduled
	void ProcessAllUnloadQueues();
	bool AreResourcesEmpty() const { return myResources.empty(); }

	void UnregisterResource(GPUResource* aRes);
	virtual void DeleteResource(GPUResource* aResource);

	RenderPass* GetRenderPass(uint32_t anId) const;

	virtual void OnResize(int aWidth, int aHeight);

private:
	using ResourceMap = std::unordered_map<Resource::Id, GPUResource*>;

	tbb::concurrent_queue<Handle<GPUResource>> myCreateQueue;
	tbb::concurrent_queue<Handle<GPUResource>> myUploadQueue;
	// Not using handles since it already made here after last handle
	// got destroyed, or requested directly
	constexpr static uint8_t kQueueCount = GraphicsConfig::kMaxFramesScheduled * 2 + 1;
	RWBuffer<tbb::concurrent_queue<GPUResource*>, kQueueCount> myUnloadQueues;
	ResourceMap myResources;
	tbb::spin_mutex myResourceMutex;

	Handle<GPUModel> myFullScrenQuad;

	void ProcessGPUQueues();

	virtual GPUResource* Create(Model*, GPUResource::UsageType aUsage) = 0;
	virtual GPUResource* Create(Pipeline*, GPUResource::UsageType aUsage) = 0;
	virtual GPUResource* Create(Shader*, GPUResource::UsageType aUsage) = 0;
	virtual GPUResource* Create(Texture*, GPUResource::UsageType aUsage) = 0;

	virtual UniformBuffer* CreateUniformBufferImpl(size_t aSize) = 0;

	void SortRenderPasses();

	int myWidth = 800;
	int myHeight = 600;
	bool myUseWireframe = false;
	bool myRenderPassesNeedOrdering = false;
};