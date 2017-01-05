#include "Graphics.h"
#include <algorithm>
#include <set>
#include <fstream>

const vector<const char *> Graphics::requiredLayers = {
#ifdef _DEBUG
	"VK_LAYER_LUNARG_standard_validation"
#endif
};
const vector<const char *> Graphics::requiredExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

GLFWwindow* Graphics::window = nullptr;
vk::Instance Graphics::instance = VK_NULL_HANDLE;
vk::Device Graphics::device = VK_NULL_HANDLE;
VkDebugReportCallbackEXT Graphics::debugCallback = VK_NULL_HANDLE;
PFN_vkCreateDebugReportCallbackEXT Graphics::CreateDebugReportCallback = VK_NULL_HANDLE;
PFN_vkDestroyDebugReportCallbackEXT Graphics::DestroyDebugReportCallback = VK_NULL_HANDLE;
vk::SurfaceKHR Graphics::surface = VK_NULL_HANDLE;
Graphics::QueuesInfo Graphics::queues;
Graphics::SwapchainSupportInfo Graphics::swapInfo;
vk::SwapchainKHR Graphics::swapchain;
vector<vk::Image> Graphics::images;
vector<vk::ImageView> Graphics::imgViews;
vk::ShaderModule Graphics::vertShader, Graphics::fragShader;
vk::RenderPass Graphics::renderPass;
vk::PipelineLayout Graphics::pipelineLayout;
vk::Pipeline Graphics::pipeline;

// Public Methods
void Graphics::Init(GLFWwindow *window)
{
	// saving a reference for internal usage
	Graphics::window = window;
	CreateInstance();
	CreateSurface();
	CreateDevice();
	CreateSwapchain();
	CreateRenderPass();
	CreatePipeline();
}

void Graphics::Render()
{
}

void Graphics::Display()
{
}

void Graphics::CleanUp()
{
	device.destroyPipeline(pipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);
	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
	for (auto v : imgViews)
		device.destroyImageView(v);
	device.destroySwapchainKHR(swapchain);
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
	if (LayersAvailable(requiredLayers))
	{
		instInfo.enabledLayerCount = requiredLayers.size();
		instInfo.ppEnabledLayerNames = requiredLayers.data();

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
	set<uint32_t> used;
	for (auto props : queueFamProps)
	{
		if (props.queueFlags == vk::QueueFlagBits::eGraphics)
		{
			queues.graphicsFamIndex = i;
			used.insert(i);
		}
		else if (props.queueFlags == vk::QueueFlagBits::eCompute)
		{
			queues.computeFamIndex = i;
			used.insert(i);
		}
		else if (props.queueFlags == vk::QueueFlagBits::eTransfer)
		{
			queues.transferFamIndex = i;
			used.insert(i);
		}
		if (pickedDevice.getSurfaceSupportKHR(i, surface))
			queues.presentFamIndex = i;
		i++;
	}
	// this time, if we have something not set find a suitable generic family
	// additionally, if multiple fitting families are found, try to use different ones
	i = 0;
	for (auto props : queueFamProps)
	{
		if (props.queueFlags & vk::QueueFlagBits::eGraphics && (queues.graphicsFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			queues.graphicsFamIndex = i;
			used.insert(i);
		}
		if (props.queueFlags & vk::QueueFlagBits::eCompute && (queues.computeFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			queues.computeFamIndex = i;
			used.insert(i);
		}
		if (props.queueFlags & vk::QueueFlagBits::eTransfer && (queues.transferFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			queues.transferFamIndex = i;
			used.insert(i);
		}
		i++;
	}
	printf("[Info] Using queue families: graphics=%d, compute=%d and transfer=%d\n", queues.graphicsFamIndex, queues.computeFamIndex, queues.transferFamIndex);

	// proper set-up of queues is a bit convoluted, cause it has to take care of the 3! cases of combinations
	vector<vk::DeviceQueueCreateInfo> queuesToCreate;
	// graphics queue should always be present
	const float priority = 1; // for simplicity's sake, priority is always the same
	for (auto fam : used)
		queuesToCreate.push_back({ {}, fam, 1, &priority });
	
	printf("[Info] Creating %d queues\n", used.size());

	// now, finally the logical device creation
	vk::PhysicalDeviceFeatures features;
	vk::DeviceCreateInfo devCreateInfo{
		{},
		(uint32_t)queuesToCreate.size(), queuesToCreate.data(),
		(uint32_t)requiredLayers.size(), requiredLayers.data(),
		(uint32_t)requiredExtensions.size(), requiredExtensions.data(),
		&features
	};
	device = pickedDevice.createDevice(devCreateInfo);

	// queues are auto-created on device creation, so just have to get their handles
	// shared queues will deal with parallelization with device events
	queues.graphicsQueue = device.getQueue(queues.graphicsFamIndex, 0);
	queues.computeQueue = device.getQueue(queues.computeFamIndex, 0);
	queues.transferQueue = device.getQueue(queues.transferFamIndex, 0);
	queues.presentQueue = device.getQueue(queues.presentFamIndex, 0);
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
	// good thing is we can still keep the vulkan.hpp definitions by manually constructing them
	surface = vk::SurfaceKHR(vkSurface);
}

void Graphics::CreateSwapchain()
{
	// by this point we have the capabilities of the surface queried, we just have to pick one
	// device might be able to support all the formats, so check if it's the case
	vk::SurfaceFormatKHR format;
	if(swapInfo.suppFormats.size() == 1 && swapInfo.suppFormats[0].format == vk::Format::eUndefined)
		format = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	else 
	{
		// device only selects a certain set of formats, try to find ours
		bool wasFound = false;
		for (auto f : swapInfo.suppFormats)
		{
			if (f.format == vk::Format::eB8G8R8A8Unorm && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				wasFound = true;
				format = f;
				break;
			}
		}
		// we couldn't find ours, so just use the first available
		if (!wasFound)
			format = swapInfo.suppFormats[0];
	}
	swapInfo.imgFormat = format.format;

	// now to figure out the present mode. mailbox is the best(vsync off), then fifo(on), then immediate(off)
	// only fifo is guaranteed to be present on all devices
	vk::PresentModeKHR mode = vk::PresentModeKHR::eFifo;
	for (auto m : swapInfo.presentModes)
	{
		if (m == vk::PresentModeKHR::eMailbox)
		{
			mode = m;
			break;
		}
	}

	// lastly (almost), need to find the swap extent(swapchain image resolutions)
	// vulkan window surface may provide us with the ability to set our own extents
	if (swapInfo.surfCaps.currentExtent.width == UINT32_MAX)
	{
		uint32_t width  = clamp(SCREEN_W, swapInfo.surfCaps.minImageExtent.width,  swapInfo.surfCaps.maxImageExtent.width);
		uint32_t height = clamp(SCREEN_H, swapInfo.surfCaps.minImageExtent.height, swapInfo.surfCaps.maxImageExtent.height);
		swapInfo.swapExtent = { width, height };
	}
	else // or it might not and we have to use it's provided extent
		swapInfo.swapExtent = swapInfo.surfCaps.currentExtent;

	// the actual last step is the determination of the image count in the swapchain
	// we want tripple buffering, but have to make sure that it's actually supported
	uint32_t imageCount = 3;
	// checking if we're in range of the caps
	if (imageCount < swapInfo.surfCaps.minImageCount)
		imageCount = swapInfo.surfCaps.minImageCount; // maybe it supports minimum 4 (strange, but have to account for strange)
	else if (swapInfo.surfCaps.maxImageCount > 0 && imageCount > swapInfo.surfCaps.maxImageCount) // 0 means only bound by device memory
		imageCount = swapInfo.surfCaps.maxImageCount;
	
	printf("[Info] Swapchain params: format=%d, colorSpace=%d, presentMode=%d, extent={%d,%d}, imgCount=%d\n",
		swapInfo.imgFormat, format.colorSpace, mode, swapInfo.swapExtent.width, swapInfo.swapExtent.height, imageCount);

	// time to fill up the swapchain creation information
	vk::SwapchainCreateInfoKHR swapCreateInfo;
	swapCreateInfo.surface = surface;
	swapCreateInfo.minImageCount = imageCount;
	swapCreateInfo.imageFormat = swapInfo.imgFormat;
	swapCreateInfo.imageColorSpace = format.colorSpace;
	swapCreateInfo.imageExtent = swapInfo.swapExtent;
	swapCreateInfo.imageArrayLayers = 1;
	swapCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; // change this later to eTransferDst to enable image-to-image copy op
	// because present family might be different from graphics family we might need to share the images
	if (queues.graphicsFamIndex != queues.presentFamIndex)
	{
		uint32_t indices[] = { queues.graphicsFamIndex, queues.presentFamIndex };
		swapCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapCreateInfo.queueFamilyIndexCount = 2;
		swapCreateInfo.pQueueFamilyIndices = indices;
	}
	else
		swapCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapCreateInfo.presentMode = mode;
	swapCreateInfo.clipped = VK_TRUE;
	swapCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	swapchain = device.createSwapchainKHR(swapCreateInfo);

	// swapchain creates the images for us to render to
	images = device.getSwapchainImagesKHR(swapchain);
	printf("[Info] Images acquired: %d\n", images.size());

	// but in order to use them, we need imageviews
	for(int i=0; i<images.size(); i++)
	{
		vk::ImageViewCreateInfo viewCreateInfo;
		viewCreateInfo.viewType = vk::ImageViewType::e2D;
		viewCreateInfo.image = images[i];
		viewCreateInfo.format = swapInfo.imgFormat;
		viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewCreateInfo.subresourceRange.levelCount = 1; // no mipmaps
		viewCreateInfo.subresourceRange.layerCount = 1; // no layers
		imgViews.push_back(device.createImageView(viewCreateInfo));
	}
}

void Graphics::CreateRenderPass()
{
	// in order to define a render pass, we need to collect all attachments for it
	// as of now we only render color to single texture
	vk::AttachmentDescription colorAttachment{
		{},
		swapInfo.imgFormat, vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
	};
	// in order to reference what attachment will be referenced in a subpass, attachment reference needs to be created
	vk::AttachmentReference colorRef{
		0, vk::ImageLayout::eColorAttachmentOptimal
	};
	// create the only subpass we need for color rendering
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	// now that we have all the required info, create the render pass
	vk::RenderPassCreateInfo passCreateInfo{
		{},
		1, &colorAttachment, // attachments
		1, &subpass, // subpasses
		0, nullptr // dependencies
	};
	renderPass = device.createRenderPass(passCreateInfo);
}

void Graphics::CreatePipeline()
{
	// first, getting the shaders
	vector<char> frag = readFile("assets/base-frag.spv");
	vector<char> vert = readFile("assets/base-vert.spv");

	// and wrapping them in to something we can pass around vulkan apis
	vertShader = device.createShaderModule({ {}, vert.size(), (const uint32_t*)vert.data() });
	fragShader = device.createShaderModule({ {}, frag.size(), (const uint32_t*)frag.data() });

	// now, to link them up
	vector<vk::PipelineShaderStageCreateInfo> stages = {
		{ {}, vk::ShaderStageFlagBits::eVertex, vertShader, "main", nullptr },
		{ {}, vk::ShaderStageFlagBits::eFragment, fragShader, "main", nullptr }
	};

	// one part of the pipeline defined, next: vertex input
	vk::PipelineVertexInputStateCreateInfo vertInputInfo{
		{},
		0, nullptr, // vert binding descriptions
		0, nullptr  // vert attribs descriptions
	};

	// then input assembly ...
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE
	};

	// viewport ...
	vk::Viewport viewport{
		0, 0,
		(float)swapInfo.swapExtent.width, (float)swapInfo.swapExtent.height,
		0, 1
	};

	// scissors ...
	vk::Rect2D scissors{
		{0, 0},
		swapInfo.swapExtent
	};

	// viewport definition from the viewport and scissors...
	vk::PipelineViewportStateCreateInfo viewportCreateInfo{
		{},
		1, &viewport,
		1, &scissors
	};

	// rasterizer...
	vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo;
	// most of the default stuff suits us, just need small adjustements
	rasterizerCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
	rasterizerCreateInfo.frontFace = vk::FrontFace::eClockwise;

	// multisampling...
	vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo;
	multisampleCreateInfo.minSampleShading = 1;

	// color blending...
	// blend states are done per frame buffer, since we have only 1 - we use only 1 state
	vk::PipelineColorBlendAttachmentState blendState;
	// since we don't do any blending, just turn it off and write to all
	blendState.blendEnable = VK_FALSE;
	blendState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// linking the state to the actual color blending part of pipeline
	vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &blendState;

	// almost there - mark states that we might dynamically change without recreating the pipeline
	vector<vk::DynamicState> dynStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};
	vk::PipelineDynamicStateCreateInfo dynStatesCreateInfo{
		{},
		(uint32_t)dynStates.size(), dynStates.data()
	};

	// define and create the pipeline layout
	vk::PipelineLayoutCreateInfo layoutCreateInfo{
		{},
		0, nullptr, // layouts
		0, nullptr  // push constant ranges
	};
	pipelineLayout = device.createPipelineLayout(layoutCreateInfo);

	// we have the layour and the renderpass - all that's needed for an actual pipeline
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
		{},
		(uint32_t)stages.size(), stages.data(),
		&vertInputInfo, &inputAssemblyInfo,
		nullptr, &viewportCreateInfo,
		&rasterizerCreateInfo, &multisampleCreateInfo,
		nullptr, &colorBlendCreateInfo,
		&dynStatesCreateInfo, pipelineLayout,
		renderPass, 0,
		VK_NULL_HANDLE, -1
	};
	pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCreateInfo);
}

// extend this later to use proper checking of caps
bool Graphics::IsSuitable(const vk::PhysicalDevice &device)
{
	bool isSuiting = true;

	vk::PhysicalDeviceProperties props = device.getProperties();
	isSuiting &= props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu || props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
	if (!isSuiting)
		return isSuiting;

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
	for (int i = 0; i < 4; i++)
		isSuiting &= foundQueues[i];
	if (!isSuiting)
		return isSuiting;

	// we also need the requested extensions
	set<string> reqExts(requiredExtensions.cbegin(), requiredExtensions.cend()); // has to be string for the == to work
	vector<vk::ExtensionProperties> supportedExts = device.enumerateDeviceExtensionProperties();
	for (auto ext : supportedExts)
		reqExts.erase(ext.extensionName);
	isSuiting &= reqExts.empty();
	if (!isSuiting)
		return isSuiting;

	// we found a proper device, now just need to make sure it has proper swapchain caps
	swapInfo.surfCaps = device.getSurfaceCapabilitiesKHR(surface);
	swapInfo.suppFormats = device.getSurfaceFormatsKHR(surface);
	swapInfo.presentModes = device.getSurfacePresentModesKHR(surface);

	// though it's strange to have swapchain support but no proper formats/present modes
	// have to check just to be safe - it's not stated in the api spec that it's always > 0
	isSuiting &= !swapInfo.suppFormats.empty() && !swapInfo.presentModes.empty();

	return isSuiting;
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

vector<char> Graphics::readFile(const string & filename)
{
	ifstream file(filename, ios::ate | ios::binary);
	if (!file.is_open())
	{
		printf("[Error] Failed to open file: %s\n", filename.c_str());
		return vector<char>();
	}
	
	size_t size = file.tellg();
	vector<char> buffer(size);
	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();
	return buffer;
}
