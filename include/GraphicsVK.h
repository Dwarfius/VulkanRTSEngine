#ifndef _GRAPHICS_VK_H
#define _GRAPHICS_VK_H

// Forcing this define for 32bit typesafe conversions, as in
// being able to construct c++ classes based of vulkan c handles
// theoretically this is unsafe - check vulkan.hpp for more info
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Graphics.h"
#include "Common.h"
#include "Game.h"

class GraphicsVK : public Graphics
{
public:
	GraphicsVK() {}

	void Init(vector<Terrain> terrains) override;
	void BeginGather() override;
	void Render(const Camera *cam, GameObject *go, const uint32_t threadId) override;
	void Display() override;
	void CleanUp() override;

	// this is a trampoline to the actual implementation
	static void OnWindowResized(GLFWwindow *window, int width, int height);

	vec3 GetModelCenter(ModelId m) override { return vec3(0, 0, 0); }
private:
	void LoadResources(vector<Terrain> terrains);

	void WindowResized(int width, int height);
	bool paused = false, gatherStarted = false;

	// Instance related
	vk::Instance instance;
	void CreateInstance();
	
	// Device related
	const static vector<const char *> requiredLayers;
	const static vector<const char *> requiredExtensions;
	vk::Device device;
	vk::PhysicalDevice physDevice;
	vk::PhysicalDeviceLimits limits;
	void CreateDevice();
	bool IsSuitable(const vk::PhysicalDevice &device);

	// Queues from Device
	struct QueuesInfo {
		uint32_t graphicsFamIndex, computeFamIndex, transferFamIndex, presentFamIndex;
		vk::Queue graphicsQueue, computeQueue, transferQueue, presentQueue;
	} queues;

	// Window surface
	vk::SurfaceKHR surface;
	void CreateSurface();

	// Swapchain
	struct SwapchainSupportInfo {
		vk::SurfaceCapabilitiesKHR surfCaps;
		vector<vk::SurfaceFormatKHR> suppFormats;
		vector<vk::PresentModeKHR> presentModes;

		// used swapchain settings
		vk::Format imgFormat;
		vk::Extent2D swapExtent;
	};
	SwapchainSupportInfo swapInfo;
	vk::SwapchainKHR swapchain = VK_NULL_HANDLE;
	vector<vk::Image> images; // images to render to, auto-destroyed by swapchain
	vector<vk::ImageView> imgViews; // views to acess images through, needs to be destroyed
	void CreateSwapchain();

	// Render Passes
	vk::RenderPass renderPass;
	void CreateRenderPass();

	// Graphic Pipeline
	vector<vk::Pipeline> pipelines;
	vk::PipelineLayout pipelineLayout;
	vk::ShaderModule vertShader, fragShader;
	vk::Pipeline CreatePipeline(string name);

	// Render Frame Buffers
	uint32_t currImgIndex;
	vector<vk::Framebuffer> swapchainFrameBuffers;
	void CreateFrameBuffers();

	// Command Pool, Buffers and Semaphores
	vk::Semaphore imgAvailable, renderFinished;
	array<vk::Fence, 3> cmdFences;
	vk::CommandPool graphCmdPool; 
	// vk::CommandPool transfCmdPool; for now we only use the generic queue
	vector<vk::CommandPool> graphSecCmdPools;
	vector<vk::CommandBuffer> cmdBuffers; // for now we have a buffer per swapchain fbo
	// it's a little strange, but essentially per each pipeline there's a secondary command buffer
	// each thread has it's own version of pipeline-linked cmd buffer
	// while also supporting triple buffering
	vector<vk::CommandBuffer> secCmdBuffers[maxThreads][3]; // for recording 
	void CreateCommandResources();

	// Depth texture
	vk::Format depthFormat;
	vk::Image depthImg;
	vk::ImageView depthImgView;
	vk::DeviceMemory depthImgMem;
	void CreateDepthTexture();
	const vector<vk::Format> depthCands = { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat };
	vk::Format FindSupportedFormat(vk::ImageTiling tiling, vk::FormatFeatureFlags feats);
	bool HasStencilComponent(vk::Format f) { return f == vk::Format::eD32SfloatS8Uint || f == vk::Format::eD24UnormS8Uint; }

	// Descriptor Sets
	struct MatUBO {
		mat4 model;
		mat4 mvp;
	};
	vk::DescriptorSetLayout uboLayout, samplerLayout;
	vk::DescriptorPool descriptorPool;
	vector<vk::DescriptorSet> uboSets, samplerSets;
	void CreateDescriptorPool();
	void CreateDescriptorSet();

	void *mappedUboMem = nullptr;
	vk::Buffer ubo;
	vk::DeviceMemory uboMem;
	void CreateUBO();
	void DestroyUBO();
	size_t GetAlignedOffset(size_t ind, size_t step) { return ind * ceil(step * 1.f / limits.minUniformBufferOffsetAlignment) * limits.minUniformBufferOffsetAlignment; }

	// Textures
	vk::Sampler sampler;
	vector<vk::ImageView> textureViews;
	vector<vk::Image> textures;
	vector<vk::DeviceMemory> textureMems;
	void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProps, vk::Image &img, vk::DeviceMemory &mem);
	void TransitionLayout(vk::Image img, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void CopyImage(vk::Buffer srcBuffer, vk::Image dstImage, uint32_t width, uint32_t height);
	vk::ImageView CreateImageView(vk::Image img, vk::Format format, vk::ImageAspectFlags aspect);
	void CreateSampler();

	// Renderable resources
	vk::VertexInputBindingDescription GetBindingDescription() const;
	array<vk::VertexInputAttributeDescription, 3> GetAttribDescriptions() const;
	// we're gonna push everything to a single vbo/ibo
	// LoadResources handles the vbo/ibo creation
	vk::Buffer vbo, ibo;
	vk::DeviceMemory vboMem, iboMem;
	vector<Model> models;
	void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProps, vk::Buffer &buff, vk::DeviceMemory &mem);
	void CopyBuffer(vk::Buffer from, vk::Buffer to, vk::DeviceSize size);
	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags props);

	// util buffer func
	vk::CommandBuffer CreateOneTimeCmdBuffer(vk::CommandBufferLevel level);
	void EndOneTimeCmdBuffer(vk::CommandBuffer cmdBuff);

	// Validation Layers related
	// Vulkan C++ binding doesn't have complete extension linking yet, so have to do it manually
	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;

	VkDebugReportCallbackEXT debugCallback;
	bool LayersAvailable(const vector<const char*> &validationLayers);
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);
};

#endif // !_GRAPHICS_VK_H
