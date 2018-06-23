#pragma once

#include "Graphics.h"
#include "TrippleBuffer.h"

class Camera;
class GameObject;

// TODO: need to implement RAII for VK types
class GraphicsVK : public Graphics
{
public:
	GraphicsVK();

	void Init(const vector<Terrain*>& aTerrainList) override;
	void BeginGather() override;
	void Render(const Camera& aCam, const GameObject* aGO) override;
	void Display() override;
	void CleanUp() override;

	// TODO: need to try to get rid of this and automate it
	void SetMaxThreads(uint32_t aMaxThreadCount) override;

	// TODO: replace the static trampoline with the bound functor object
	// this is a trampoline to the actual implementation
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);

private:
	uint32_t myMaxThreads;
	int myThreadCounter;
	tbb::spin_mutex myThreadCounterSpinlock;
	tbb::enumerable_thread_specific<glm::uint> myThreadLocalIndices;

	void LoadResources(const vector<Terrain*>& aTerrainList);

	void WindowResized(int aWidth, int aHeight);
	bool myIsPaused, myGatherStarted;

	// Instance related
	vk::Instance myInstance;
	void CreateInstance();
	
	// Device related
	const static vector<const char *> ourRequiredLayers;
	const static vector<const char *> ourRequiredExtensions;
	vk::Device myDevice;
	vk::PhysicalDevice myPhysDevice;
	vk::PhysicalDeviceLimits myLimits;
	void CreateDevice();
	bool IsSuitable(const vk::PhysicalDevice& aDevice);

	// Queues from Device
	struct QueuesInfo 
	{
		uint32_t myGraphicsFamIndex, myComputeFamIndex, myTransferFamIndex, myPresentFamIndex;
		vk::Queue myGraphicsQueue, myComputeQueue, myTransferQueue, myPresentQueue;
	};
	QueuesInfo myQueues;

	// Window surface
	vk::SurfaceKHR mySurface;
	void CreateSurface();

	// Swapchain
	struct SwapchainSupportInfo 
	{
		vk::SurfaceCapabilitiesKHR mySurfCaps;
		vector<vk::SurfaceFormatKHR> mySupportedFormats;
		vector<vk::PresentModeKHR> myPresentModes;

		// used swapchain settings
		vk::Format myImgFormat;
		vk::Extent2D mySwapExtent;
	};
	SwapchainSupportInfo mySwapInfo;
	vk::SwapchainKHR mySwapchain;
	vector<vk::Image> myImages; // images to render to, auto-destroyed by swapchain
	vector<vk::ImageView> myImgViews; // views to acess images through, needs to be destroyed
	void CreateSwapchain();

	// Render Passes
	vk::RenderPass myRenderPass;
	void CreateRenderPass();

	// Graphic Pipeline
	vector<vk::Pipeline> myPipelines;
	vk::PipelineLayout myPipelineLayout;
	vector<vk::ShaderModule> myShaderModules;
	vk::Pipeline CreatePipeline(string aName);

	// Render Frame Buffers
	uint32_t myCurrentImageIndex;
	vector<vk::Framebuffer> mySwapchainFrameBuffers;
	void CreateFrameBuffers();

	// Command Pool, Buffers and Semaphores
	vk::Semaphore myImgAvailable, myRenderFinished;
	TrippleBuffer<vk::Fence> myCmdFences;
	vk::CommandPool myGraphCmdPool; 
	// vk::CommandPool transfCmdPool; for now we only use the generic queue
	vector<vk::CommandPool> myGraphSecCmdPools;
	TrippleBuffer<vk::CommandBuffer> myCmdBuffers; // for now we use tripple buffer
	// it's a little strange, but essentially per each pipeline there's a secondary command buffer
	// each thread has it's own version of pipeline-linked cmd buffer
	// while also supporting triple buffering
	typedef vector<vk::CommandBuffer> PerPipelineCmdBuffers;
	typedef vector<PerPipelineCmdBuffers> PerThreadCmdBuffers;
	TrippleBuffer<PerThreadCmdBuffers> mySecCmdBuffers; // for recording drawcalls per image per thread per pipeline
	void CreateCommandResources();

	// Depth texture
	vk::Format myDepthFormat;
	vk::Image myDepthImg;
	vk::ImageView myDepthImgView;
	vk::DeviceMemory myDepthImgMem;
	void CreateDepthTexture();
	const vector<vk::Format> myDepthCands = { 
		vk::Format::eD24UnormS8Uint, 
		vk::Format::eD32SfloatS8Uint, 
		vk::Format::eD32Sfloat 
	};
	vk::Format FindSupportedFormat(vk::ImageTiling aTiling, vk::FormatFeatureFlags aFeatures) const;
	bool HasStencilComponent(vk::Format aFormat) const { return aFormat == vk::Format::eD32SfloatS8Uint || aFormat == vk::Format::eD24UnormS8Uint; }

	// Descriptor Sets - tripple buffered to avoid syncing
	tbb::spin_mutex mySlotIndexMutex;
	size_t mySlotIndex;
	struct MatUBO {
		glm::mat4 myModel;
		glm::mat4 myMvp;
	};
	vk::DescriptorSetLayout myUboLayout, mySamplerLayout;
	vk::DescriptorPool myDescriptorPool;
	typedef vector<vk::DescriptorSet> DescriptorSets;
	TrippleBuffer<DescriptorSets> myUboSets;
	DescriptorSets mySamplerSets;
	void CreateDescriptorPool();
	void CreateDescriptorSet();

	// since descriptors are written to mapped mem, it also has to be tripple buffered
	TrippleBuffer<void*> myMappedUboMem;
	vk::Buffer myUbo;
	vk::DeviceMemory myUboMem;
	void CreateUBO();
	void DestroyUBO();
	size_t GetAlignedOffset(size_t anInd, size_t aStep) const;

	// Textures
	vk::Sampler mySampler;
	vector<vk::ImageView> myTextureViews;
	vector<vk::Image> myTextures;
	vector<vk::DeviceMemory> myTextureMems;
	void CreateImage(uint32_t aWidth, uint32_t aHeight, vk::Format aFormat, vk::ImageTiling aTiling, vk::ImageUsageFlags aUsage, vk::MemoryPropertyFlags aMemProps, vk::Image& anImg, vk::DeviceMemory& aMem) const;
	void TransitionLayout(vk::Image anImg, vk::Format aFormat, vk::ImageLayout anOldLayout, vk::ImageLayout aNewLayout) const;
	void CopyImage(vk::Buffer aSrcBuffer, vk::Image aDstImage, uint32_t aWidth, uint32_t aHeight) const;
	vk::ImageView CreateImageView(vk::Image anImg, vk::Format aFormat, vk::ImageAspectFlags anAspect) const;
	void CreateSampler();

	// Renderable resources
	vk::VertexInputBindingDescription GetBindingDescription() const;
	array<vk::VertexInputAttributeDescription, 3> GetAttribDescriptions() const;
	// we're gonna push everything to a single vbo/ibo
	// LoadResources handles the vbo/ibo creation
	vk::Buffer myVbo, myIbo;
	vk::DeviceMemory myVboMem, myIboMem;
	void CreateBuffer(vk::DeviceSize aSize, vk::BufferUsageFlags aUsage, vk::MemoryPropertyFlags aMemProps, vk::Buffer& aBuff, vk::DeviceMemory& aMem) const;
	void CopyBuffer(vk::Buffer aFrom, vk::Buffer aTo, vk::DeviceSize aSize) const;
	uint32_t FindMemoryType(uint32_t aTypeFilter, vk::MemoryPropertyFlags aProps) const;

	// util buffer func
	vk::CommandBuffer CreateOneTimeCmdBuffer(vk::CommandBufferLevel aLevel) const;
	void EndOneTimeCmdBuffer(vk::CommandBuffer aCmdBuff) const;

	// Validation Layers related
	// Vulkan C++ binding doesn't have complete extension linking yet, so have to do it manually
	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;

	VkDebugReportCallbackEXT myDebugCallback;
	bool LayersAvailable(const vector<const char*>& aValidationLayersList) const;
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT aFlags,
		VkDebugReportObjectTypeEXT anObjType,
		uint64_t aObj,
		size_t aLocation,
		int32_t aCode,
		const char* aLayerPrefix,
		const char* aMsg,
		void* aUserData);
};
