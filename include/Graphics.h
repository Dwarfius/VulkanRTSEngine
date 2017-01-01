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
	static void Init();
	static void Render();
	static void Display();
	static void CleanUp();

	static vk::Instance GetInstance() { return instance; }
	static vk::Device GetDevice() { return device; }
private:
	Graphics();

	static vk::Instance instance;
	static vk::Device device;

	// Validation Layers related
	// Vulkan C++ binding doesn't have complete extension linking yet, so have to do it manually
	static PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	static PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;

	static VkDebugReportCallbackEXT debugCallback;
	static bool Graphics::LayersAvailable(const vector<const char*> validationLayers);
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
