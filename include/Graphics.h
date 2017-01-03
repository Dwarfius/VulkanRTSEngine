#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "Common.h"

// This class creates an interface with the Vulkan
// rendering system. Goal is to hide away the 
// render-api implementation so that in the future it can
// be extended with other render-apis.

class Graphics
{
public:
	static void Init(GLFWwindow *window);
	static void Render();
	static void Display();
	static void CleanUp();

	static vk::Instance* GetInstance() { return &instance; }
	static vk::Device* GetDevice() { return &device; }
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
};

#endif // !_GRAPHICS_H
