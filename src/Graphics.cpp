#include "Graphics.h"

vk::Instance Graphics::instance;
vk::Device Graphics::device;
VkDebugReportCallbackEXT Graphics::debugCallback = VK_NULL_HANDLE;
PFN_vkCreateDebugReportCallbackEXT Graphics::CreateDebugReportCallback = VK_NULL_HANDLE;
PFN_vkDestroyDebugReportCallbackEXT Graphics::DestroyDebugReportCallback = VK_NULL_HANDLE;

void Graphics::Init()
{
	// creating the instance
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "VulkanRTSEngine";
	appInfo.pEngineName = "No Engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	vk::InstanceCreateInfo instInfo;
	instInfo.pApplicationInfo = &appInfo;
	// first, the extensions
	vector<const char *> extensions;
	uint32_t extensionCount;
	const char **baseExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	for (uint32_t i = 0; i < extensionCount; i++)
		extensions.push_back(baseExtensions[i]);

#ifdef _DEBUG
	const vector<const char*> layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	if (LayersAvailable(layers))
	{
		instInfo.enabledLayerCount = layers.size();
		instInfo.ppEnabledLayerNames = layers.data();

		// in order to receive the messages, we need a callback extension
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
#endif

	instInfo.enabledExtensionCount = extensions.size();
	instInfo.ppEnabledExtensionNames = extensions.data();

	instance = vk::createInstance(instInfo);

#ifdef _DEBUG
	CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT");
	DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
	
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)DebugCallback;
	
	CreateDebugReportCallback(instance, &callbackCreateInfo, nullptr, &debugCallback);
#endif
}

void Graphics::Render()
{

}

void Graphics::Display()
{
}

void Graphics::CleanUp()
{
	DestroyDebugReportCallback(instance, debugCallback, nullptr);
	instance.destroy();
}

bool Graphics::LayersAvailable(const vector<const char*> validationLayers)
{
	//what we have
	uint32_t availableLayersCount;
	vk::enumerateInstanceLayerProperties(&availableLayersCount, nullptr);

	vector<vk::LayerProperties> availableLayers(availableLayersCount);
	vk::enumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());

	// validation layers lookup
	bool foundAll = true;
	for each (auto layerRequested in validationLayers)
	{
		bool found = false;
		for each(auto layerAvailable in availableLayers)
		{
			if (strcmp(layerAvailable.layerName, layerRequested) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			printf("Warning - didn't find validation layer: %s\n", layerRequested);
			foundAll = false;
			break;
		}
	}
	return foundAll;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Graphics::DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData)
{
	string type;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		fprintf(stderr, "[Error] Validation layer: %s\n", msg);
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		fprintf(stderr, "[Warning] Validation layer: %s\n", msg);
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		fprintf(stderr, "[Performance] Validation layer: %s\n", msg);
	return VK_FALSE;
}
