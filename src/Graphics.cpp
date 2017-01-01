#include "Graphics.h"
#include <algorithm>
#include <set>

GLFWwindow* Graphics::window = nullptr;
vk::Instance Graphics::instance = VK_NULL_HANDLE;
vk::Device Graphics::device = VK_NULL_HANDLE;
uint32_t Graphics::graphicsFamIndex = UINT32_MAX, Graphics::computeFamIndex = UINT32_MAX, Graphics::transferFamIndex = UINT32_MAX;
vk::Queue Graphics::graphicsQueue = VK_NULL_HANDLE, Graphics::computeQueue = VK_NULL_HANDLE, Graphics::transferQueue = VK_NULL_HANDLE, Graphics::presentQueue = VK_NULL_HANDLE;
VkDebugReportCallbackEXT Graphics::debugCallback = VK_NULL_HANDLE;
PFN_vkCreateDebugReportCallbackEXT Graphics::CreateDebugReportCallback = VK_NULL_HANDLE;
PFN_vkDestroyDebugReportCallbackEXT Graphics::DestroyDebugReportCallback = VK_NULL_HANDLE;
vk::SurfaceKHR Graphics::surface = VK_NULL_HANDLE;

// Public Methods
void Graphics::Init(GLFWwindow *window)
{
	// saving a reference for internal usage
	Graphics::window = window;
	CreateInstance();
	CreateSurface();
	CreateDevice();
}

void Graphics::Render()
{
}

void Graphics::Display()
{
}

void Graphics::CleanUp()
{
	instance.destroySurfaceKHR(surface);
	device.destroy();
	DestroyDebugReportCallback(instance, debugCallback, nullptr);
	instance.destroy();
}

// Private Methods
void Graphics::CreateInstance()
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

void Graphics::CreateDevice()
{
	// first we gotta find our physical device before we can create a logical one
	vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
	if (devices.size() == 0)
	{
		printf("[Error] No vulkan devices found!");
		return;
	}

	printf("[Info] Found %d vulkan devices\n", devices.size());
	vk::PhysicalDevice pickedDevice = VK_NULL_HANDLE;
	for (auto device : devices)
	{
		if (IsSuitable(device))
		{
			pickedDevice = device;
			break;
		}
	}
	if (!pickedDevice)
	{
		printf("[Error] No suitable vulkan device found!");
		return;
	}

	// then we gotta pick queue families for our use (graphics, compute and transfer)
	vector<vk::QueueFamilyProperties> queueFamProps = pickedDevice.getQueueFamilyProperties();
	int i = 0;
	/*
	// for now, using the simple set-up with 1 queue
	for each(auto props in queueFamProps)
	{
		if (props.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			graphicsFamIndex = i;
			break;
		}
		i++;
	}
	*/
	// first going to attemp to find specialized queues
	// this way we can better utilize gpu
	uint32_t presentFamIndex;
	set<uint32_t> used;
	for (auto props : queueFamProps)
	{
		if (props.queueFlags == vk::QueueFlagBits::eGraphics)
		{
			graphicsFamIndex = i;
			used.insert(i);
		}
		else if (props.queueFlags == vk::QueueFlagBits::eCompute)
		{
			computeFamIndex = i;
			used.insert(i);
		}
		else if (props.queueFlags == vk::QueueFlagBits::eTransfer)
		{
			transferFamIndex = i;
			used.insert(i);
		}
		if (pickedDevice.getSurfaceSupportKHR(i, surface))
			presentFamIndex = i;
		i++;
	}
	// this time, if we have something not set find a suitable generic family
	// additionally, if multiple fitting families are found, try to use different ones
	i = 0;
	for (auto props : queueFamProps)
	{
		if (props.queueFlags & vk::QueueFlagBits::eGraphics && (graphicsFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			graphicsFamIndex = i;
			used.insert(i);
		}
		if (props.queueFlags & vk::QueueFlagBits::eCompute && (computeFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			computeFamIndex = i;
			used.insert(i);
		}
		if (props.queueFlags & vk::QueueFlagBits::eTransfer && (transferFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			transferFamIndex = i;
			used.insert(i);
		}
		i++;
	}
	printf("[Info] Using graphics=%d, compute=%d and transfer=%d\n", graphicsFamIndex, computeFamIndex, transferFamIndex);

	// proper set-up of queues is a bit convoluted, cause it has to take care of the 3! cases of combinations
	vector<vk::DeviceQueueCreateInfo> queuesToCreate;
	// graphics queue should always be present
	const float priority = 1; // for simplicity's sake, priority is always the same
	for (auto fam : used)
		queuesToCreate.push_back({ {}, fam, 1, &priority });
	
	printf("[Info] Creating %d queues\n", used.size(), graphicsFamIndex);

	// activating same layers for device
	const vector<const char*> layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	// now, finally the logical device creation
	vk::PhysicalDeviceFeatures features;
	vk::DeviceCreateInfo devCreateInfo{
		{},
		(uint32_t)queuesToCreate.size(), queuesToCreate.data(),
		(uint32_t)layers.size(), layers.data(),
		0, nullptr,
		&features
	};
	device = pickedDevice.createDevice(devCreateInfo);

	// queues are auto-created on device creation, so just have to get their handles
	// shared queues will deal with parallelization with device events
	graphicsQueue = device.getQueue(graphicsFamIndex, 0);
	computeQueue = device.getQueue(computeFamIndex, 0);
	transferQueue = device.getQueue(transferFamIndex, 0);
	presentQueue = device.getQueue(presentFamIndex, 0);
}

void Graphics::CreateSurface()
{
	// another vulkan.hpp issue - can't get underlying pointer, have to manually construct surface
	VkSurfaceKHR vkSurface;
	if (glfwCreateWindowSurface(instance, window, nullptr, &vkSurface) != VK_SUCCESS)
	{
		printf("[Error] Surface creation failed");
		return;
	}
	surface = vk::SurfaceKHR(vkSurface);
}

// extend this later to use proper checking of caps
bool Graphics::IsSuitable(const vk::PhysicalDevice &device)
{
	vk::PhysicalDeviceProperties props = device.getProperties();
	//vk::PhysicalDeviceFeatures feats = device.getFeatures();

	// we need graphics, compute, transfer and present (can be same queue family)
	vector<vk::QueueFamilyProperties> queueFamProps = device.getQueueFamilyProperties();
	bool foundQueues[4] = { false, false, false, false };
	uint32_t i = 0;
	for (auto prop : queueFamProps)
	{
		if (prop.queueFlags & vk::QueueFlagBits::eGraphics)
			foundQueues[0] = true;
		if (prop.queueFlags & vk::QueueFlagBits::eCompute)
			foundQueues[1] = true;
		if (prop.queueFlags & vk::QueueFlagBits::eTransfer)
			foundQueues[2] = true;
		if (device.getSurfaceSupportKHR(i, surface))
			foundQueues[3] = true;
		i++;
	}
	bool allQueuesPresent = true;
	for (int i = 0; i < 4; i++)
		allQueuesPresent &= foundQueues[i];

	return allQueuesPresent && (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu || props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu);
}

bool Graphics::LayersAvailable(const vector<const char*> &validationLayers)
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
			printf("[Warning] Didn't find validation layer: %s\n", layerRequested);
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
		printf("[Error] VL: %s\n", msg);
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		printf("[Warning] VL: %s\n", msg);
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		printf("[Performance] VL: %s\n", msg);
	return VK_FALSE;
}
