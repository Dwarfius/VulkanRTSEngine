#ifndef _GRAPHICS_VK_H
#define _GRAPHICS_VK_H

#include "Common.h"

class Graphics
{
public:
	static void Init();
	static void Render();
	static void Display();
	static void CleanUp();

	static GLFWwindow* GetWindow() { return window; }

	static void OnWindowResized(GLFWwindow *window, int width, int height);
private:
	Graphics();

	static GLFWwindow *window;

	// Instance related
	static vk::Instance instance;
	static void CreateInstance();
	
	// Device related
	const static vector<const char *> requiredLayers;
	const static vector<const char *> requiredExtensions;
	static vk::Device device;
	static void CreateDevice();
	static bool IsSuitable(const vk::PhysicalDevice &device);

	// Queues from Device
	struct QueuesInfo {
		uint32_t graphicsFamIndex, computeFamIndex, transferFamIndex, presentFamIndex;
		vk::Queue graphicsQueue, computeQueue, transferQueue, presentQueue;
	};
	static QueuesInfo queues;

	// Window surface
	static vk::SurfaceKHR surface;
	static void CreateSurface();

	// Swapchain
	struct SwapchainSupportInfo {
		vk::SurfaceCapabilitiesKHR surfCaps;
		vector<vk::SurfaceFormatKHR> suppFormats;
		vector<vk::PresentModeKHR> presentModes;

		// used swapchain settings
		vk::Format imgFormat;
		vk::Extent2D swapExtent;
	};
	static SwapchainSupportInfo swapInfo;
	static vk::SwapchainKHR swapchain;
	static vector<vk::Image> images; // images to render to, auto-destroyed by swapchain
	static vector<vk::ImageView> imgViews; // views to acess images through, needs to be destroyed
	static void CreateSwapchain();

	// Render Passes
	static vk::RenderPass renderPass;
	static void CreateRenderPass();

	// Graphic Pipeline
	static vk::Pipeline pipeline;
	static vk::PipelineLayout pipelineLayout;
	static vk::ShaderModule vertShader, fragShader;
	static void CreatePipeline();

	// Render Frame Buffers
	static vector<vk::Framebuffer> swapchainFrameBuffers;
	static void CreateFrameBuffers();

	// Command Pool, Buffers and Semaphores
	static vk::Semaphore imgAvailable, renderFinished;
	static vk::CommandPool pool;
	static vector<vk::CommandBuffer> cmdBuffers; // for now we have a buffer ber swapchain fbo
	static void CreateCommandResources();

	// Validation Layers related
	// Vulkan C++ binding doesn't have complete extension linking yet, so have to do it manually
	static PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	static PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;

	static VkDebugReportCallbackEXT debugCallback;
	static bool Graphics::LayersAvailable(const vector<const char*> &validationLayers);
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);

	// Utilities
	static vector<char> readFile(const string& filename);
};

#endif // !_GRAPHICS_VK_H
