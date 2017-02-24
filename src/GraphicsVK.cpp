#include "GraphicsVK.h"
#include "GameObject.h"
#include <algorithm>
#include <set>
#include <fstream>

GraphicsVK* GraphicsVK::activeGraphics = NULL;

const vector<const char *> GraphicsVK::requiredLayers = {
#ifdef _DEBUG
	"VK_LAYER_LUNARG_standard_validation",
#endif
	"VK_LAYER_LUNARG_monitor"
};
const vector<const char *> GraphicsVK::requiredExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Public Methods
void GraphicsVK::Init()
{
	if (glfwVulkanSupported() == GLFW_FALSE)
	{
		printf("[Error] Vulkan loader unavailable");
		return;
	}

	activeGraphics = this;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Vulkan RTS Engine", nullptr, nullptr);
	
	glfwSetWindowSizeCallback(window, OnWindowResized);
	CreateInstance();
	CreateSurface();
	CreateDevice();
	CreateSwapchain();
	CreateRenderPass();
	CreateFrameBuffers();
	CreateCommandResources();
	CreateUBO();
	CreateDescriptorPool();
	LoadResources();
	CreateSampler();
	CreateDescriptorSet();
}

void GraphicsVK::LoadResources()
{
	// same order as GraphicsGL
	// shaders
	for (string shaderName : shadersToLoad)
	{
		vk::Pipeline pipeline = CreatePipeline(shaderName);
		pipelines.push_back(pipeline);
	}
	// continue with UBOs. don't use a DEVICE_LOCAL memory, just write to mapped directly
	// https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer

	// models
	{
		vector<Vertex> vertices;
		vector<uint32_t> indices;

		// have to perform vert and index offset calculations as well
		for (string modelName : modelsToLoad)
		{
			Model m;
			m.vertexOffset = vertices.size();
			m.indexOffset = indices.size();
			
			LoadModel(modelName, vertices, indices, m.center, m.sphereRadius);
			m.vertexCount = vertices.size() - m.vertexOffset;
			m.indexCount = indices.size() - m.indexOffset;

			models.push_back(m);
		}

		// creating staging vbo
		vk::Buffer stagingBuff;
		vk::DeviceMemory stagingMem;
		vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc;
		vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		vk::DeviceSize size = sizeof(Vertex) * vertices.size();
		CreateBuffer(size, usage, memProps, stagingBuff, stagingMem);

		// now buffer the memory to staging
		void* data = device.mapMemory(stagingMem, 0, size, {});
			memcpy(data, vertices.data(), size);
		device.unmapMemory(stagingMem);

		// creating vbo device local
		usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
		CreateBuffer(size, usage, memProps, vbo, vboMem);

		// create the command buffer to perform cross-buffer copy
		CopyBuffer(stagingBuff, vbo, size);

		// cleanup
		device.destroyBuffer(stagingBuff);
		device.freeMemory(stagingMem);

		// same for ibo
		usage = vk::BufferUsageFlagBits::eTransferSrc;
		memProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		size = sizeof(uint32_t) * indices.size();
		CreateBuffer(size, usage, memProps, stagingBuff, stagingMem);

		// now buffer the memory to staging
		data = device.mapMemory(stagingMem, 0, size, {});
			memcpy(data, indices.data(), size);
		device.unmapMemory(stagingMem);

		// creating ibo device local
		usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
		CreateBuffer(size, usage, memProps, ibo, iboMem);

		// transfer
		CopyBuffer(stagingBuff, ibo, size);

		// cleanup
		device.destroyBuffer(stagingBuff);
		device.freeMemory(stagingMem);
	}

	// textures
	{
		// thank you Sascha Williams
		// https://github.com/SaschaWillems/Vulkan/blob/master/texture/texture.cpp
		// again we need a staging buffer
		vk::Buffer stagingBuff;
		vk::DeviceMemory stagingMem;
		vk::DeviceSize stagingSize = 8096 * 8096 * 4; // 8k x 8k or RGBA32
		CreateBuffer(stagingSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuff, stagingMem);

		// start processing the textures
		textures.reserve(texturesToLoad.size());
		textureMems.reserve(texturesToLoad.size());
		for (string textureName : texturesToLoad)
		{
			// load up the texture data
			int width, height, channels;
			unsigned char *pixels = LoadTexture("assets/textures/" + textureName, &width, &height, &channels, STBI_rgb_alpha);
			vk::DeviceSize texSize = height * width * channels;

			// copy it over to buffer
			uint8_t *data = (uint8_t*)device.mapMemory(stagingMem, 0, stagingSize, {});
				memcpy(data, pixels, (size_t)texSize);
			device.unmapMemory(stagingMem);

			// create the actual texture
			vk::Image text;
			vk::DeviceMemory textMem;
			CreateImage(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, text, textMem);

			// in order to perform the copy the staging buffer has to be in TransferSrcOptimal layout
			// same for target, but TransferDstOptimal
			TransitionLayout(text, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
			// now we can copy
			CopyImage(stagingBuff, text, width, height);
			// in order to use it it has to be transitioned in to ShaderReadOnlyOptimal
			TransitionLayout(text, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

			// we gud, texture ready
			textures.push_back(text);
			textureMems.push_back(textMem);
			FreeTexture(pixels);

			// create a textureView for it
			vk::ImageView view = CreateImageView(text, vk::Format::eR8G8B8A8Unorm);
			textureViews.push_back(view);
		}

		device.destroyBuffer(stagingBuff);
		device.freeMemory(stagingMem);
	}
}

void GraphicsVK::BeginGather()
{
	// acquire image to render to
	currImgIndex = device.acquireNextImageKHR(swapchain, UINT32_MAX, imgAvailable, VK_NULL_HANDLE).value;

	// before we start recording the command buffer, need to make sure that it has finished working
	device.waitForFences(1, &cmdFences[currImgIndex], false, ~0ull);
	device.resetFences(1, &cmdFences[currImgIndex]);

	// Vulkan.hpp is still a bit of a donkey so have to manually initialize with my values through copy
	float clearColor[] = { 0, 0, 0, 1 };
	vk::ClearValue clearVal;
	memcpy(&clearVal.color.float32, clearColor, sizeof(clearVal.color.float32));

	// begin recording to command buffer
	vk::CommandBuffer primaryCmdBuffer = cmdBuffers[currImgIndex];
	primaryCmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eSimultaneousUse });

	// start the render pass
	vk::RenderPassBeginInfo beginInfo{
		renderPass,
		swapchainFrameBuffers[currImgIndex],
		{ { 0, 0 }, swapInfo.swapExtent },
		1, &clearVal
	};
	// has to be vk::SubpassContents::eSecondaryCommandBuffers
	primaryCmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

	// have to begin all secondary buffers here
	for (size_t thread = 0; thread < maxThreads; thread++)
	{
		for (size_t pipeline = 0; pipeline < shadersToLoad.size(); pipeline++)
		{
			vk::CommandBuffer buff = secCmdBuffers[thread][currImgIndex][pipeline];
			// need inheritance info since secondary buffer
			vk::CommandBufferInheritanceInfo inheritanceInfo{
				renderPass, 0, // render using subpass#0 of renderpass
				swapchainFrameBuffers[currImgIndex],
				0
			};
			vk::CommandBufferBeginInfo info{
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit 
					| vk::CommandBufferUsageFlagBits::eRenderPassContinue
					| vk::CommandBufferUsageFlagBits::eSimultaneousUse,
				&inheritanceInfo
			};
			buff.begin(info);

			// bind the vbo and ibo
			vk::DeviceSize offset = 0;
			buff.bindVertexBuffers(0, 1, &vbo, &offset);
			buff.bindIndexBuffer(ibo, 0, vk::IndexType::eUint32);

			// bind the pipeline for rendering with
			buff.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[pipeline]);
		}
	}
}

void GraphicsVK::Render(const Camera *cam, GameObject *go, const uint32_t threadId)
{
	vec3 scale = go->GetScale();
	float maxScale = max({ scale.x, scale.y, scale.z });
	float scaledRadius = models[go->GetModel()].sphereRadius * maxScale;
	if (!cam->CheckSphere(go->GetPos(), scaledRadius))
		return;

	// get the vector of secondary buffers for thread
	vector<vk::CommandBuffer> buffers = secCmdBuffers[threadId][currImgIndex];
	// we need to find the corresponding secondary buffer
	ShaderId pipelineInd = go->GetShader();
	Model m = models[go->GetModel()];
	size_t index = go->GetIndex();

	// update the uniforms
	auto uniforms = go->GetUniforms();
	MatUBO matrices;
	matrices.model = go->GetModelMatrix();
	matrices.mvp = cam->Get() * matrices.model;
	memcpy((char*)mappedUboMem + GetAlignedOffset(index, sizeof(MatUBO)), &matrices, sizeof(MatUBO));
	
	// draw out all the indices
	array<vk::DescriptorSet, 2> setsToBind{
		uboSets[index], samplerSets[go->GetTexture()]
	};
	buffers[pipelineInd].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 
		0, (uint32_t)setsToBind.size(), setsToBind.data(), 0, nullptr);
	buffers[pipelineInd].drawIndexed(m.indexCount, 1, m.indexOffset, 0, 0);
}

void GraphicsVK::Display()
{
	// before we can execute them, have to end them
	for (size_t thread = 0; thread < maxThreads; thread++)
		for (auto buffs : secCmdBuffers[thread][currImgIndex])
			buffs.end();

	// draw out all the accumulated render calls
	vk::CommandBuffer primaryCmdBuffer = cmdBuffers[currImgIndex];
	for (size_t pipelineInd = 0; pipelineInd < pipelines.size(); pipelineInd++)
	{
		vk::Pipeline pipeline = pipelines[pipelineInd];

		// bind the pipeline for rendering with
		// primaryCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		// push the secondaries in to primary
		for (size_t thread = 0; thread<maxThreads; thread++)
			primaryCmdBuffer.executeCommands(1, &secCmdBuffers[thread][currImgIndex][pipelineInd]);
	}

	// finish up the render pass
	primaryCmdBuffer.endRenderPass();

	// finish the recording
	primaryCmdBuffer.end();

	// submit the queue
	// make sure it waits for the image to become available before we start outputting color
	vk::PipelineStageFlags waitAt = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submitInfo{
		1, &imgAvailable, &waitAt,
		1, &primaryCmdBuffer,
		1, &renderFinished
	};
	queues.graphicsQueue.submit(1, &submitInfo, cmdFences[currImgIndex]);

	// present the results of the drawing
	vk::PresentInfoKHR presentInfo{
		1, &renderFinished,
		1, &swapchain, &currImgIndex,
		nullptr
	};
	queues.graphicsQueue.presentKHR(presentInfo);
}

void GraphicsVK::CleanUp()
{
	// gonna make sure all the tasks are finished before we can start destroying resources
	device.waitIdle();

	device.destroySampler(sampler);

	for (auto v : textureViews)
		device.destroyImageView(v);
	textureViews.clear();
	for (auto t : textures)
		device.destroyImage(t);
	textures.clear();
	for (auto m : textureMems)
		device.freeMemory(m);
	textureMems.clear();

	device.destroyDescriptorPool(descriptorPool);

	DestroyUBO();
	device.destroyDescriptorSetLayout(uboLayout);
	device.destroyDescriptorSetLayout(samplerLayout);

	for (auto fence : cmdFences)
		device.destroyFence(fence);
	device.destroyBuffer(vbo);
	device.freeMemory(vboMem);
	device.destroyBuffer(ibo);
	device.freeMemory(iboMem);

	device.destroySemaphore(imgAvailable);
	device.destroySemaphore(renderFinished);
	device.destroyCommandPool(graphCmdPool);
	//device.destroyCommandPool(transfCmdPool);
	for (auto pool : graphSecCmdPools)
		device.destroyCommandPool(pool);
	graphSecCmdPools.clear();
	
	for (auto b : swapchainFrameBuffers)
		device.destroyFramebuffer(b);
	swapchainFrameBuffers.clear();
	
	for(auto pipeline : pipelines)
		device.destroyPipeline(pipeline);
	pipelines.clear();
	
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);
	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);

	for (auto v : imgViews)
		device.destroyImageView(v);
	imgViews.clear();
	device.destroySwapchainKHR(swapchain);
	instance.destroySurfaceKHR(surface);

	device.destroy();
	DestroyDebugReportCallback(instance, debugCallback, nullptr);
	instance.destroy();

	glfwDestroyWindow(window);

	activeGraphics = NULL;
}

void GraphicsVK::OnWindowResized(GLFWwindow * window, int width, int height)
{
	if (width == 0 && height == 0)
		return;

	activeGraphics->WindowResized(width, height);
}

void GraphicsVK::WindowResized(int width, int height)
{
	// gonna make sure all the tasks are finished before we can start destroying resources
	device.waitIdle();
}

// Private Methods
void GraphicsVK::CreateInstance()
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

void GraphicsVK::CreateDevice()
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
	physDevice = pickedDevice;

	// then we gotta pick queue families for our use (graphics, compute and transfer)
	vector<vk::QueueFamilyProperties> queueFamProps = physDevice.getQueueFamilyProperties();
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
	device = physDevice.createDevice(devCreateInfo);

	// queues are auto-created on device creation, so just have to get their handles
	// shared queues will deal with parallelization with device events
	queues.graphicsQueue = device.getQueue(queues.graphicsFamIndex, 0);
	//queues.computeQueue = device.getQueue(queues.computeFamIndex, 0);
	//queues.transferQueue = device.getQueue(queues.transferFamIndex, 0);
	queues.presentQueue = device.getQueue(queues.presentFamIndex, 0);

	// we need the alignment for our uniform buffers
	limits = physDevice.getProperties().limits;
}

// extend this later to use proper checking of caps
bool GraphicsVK::IsSuitable(const vk::PhysicalDevice &device)
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

void GraphicsVK::CreateSurface()
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

void GraphicsVK::CreateSwapchain()
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
		imgViews.push_back(CreateImageView(images[i], swapInfo.imgFormat));
}

void GraphicsVK::CreateRenderPass()
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

	// specifying dependencies to tell Vulkan api how to transition images
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; // we only have 1 subpass, so it's 0

	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};

	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

	// now that we have all the required info, create the render pass
	vk::RenderPassCreateInfo passCreateInfo{
		{},
		1, &colorAttachment, // attachments
		1, &subpass, // subpasses
		1, &dependency // dependencies
	};
	renderPass = device.createRenderPass(passCreateInfo);
}

vk::Pipeline GraphicsVK::CreatePipeline(string name)
{
	// first, getting the shaders
	string frag = readFile("assets/VulkanShaders/" + name + "-frag.spv");
	string vert = readFile("assets/VulkanShaders/" + name + "-vert.spv");

	// and wrapping them in to something we can pass around vulkan apis
	vertShader = device.createShaderModule({ {}, vert.size(), (const uint32_t*)vert.data() });
	fragShader = device.createShaderModule({ {}, frag.size(), (const uint32_t*)frag.data() });

	// now, to link them up
	vector<vk::PipelineShaderStageCreateInfo> stages = {
		{ {}, vk::ShaderStageFlagBits::eVertex, vertShader, "main", nullptr },
		{ {}, vk::ShaderStageFlagBits::eFragment, fragShader, "main", nullptr }
	};

	// one part of the pipeline defined, next: vertex input
	vk::VertexInputBindingDescription binding = GetBindingDescription();
	array<vk::VertexInputAttributeDescription, 3> attribs = GetAttribDescriptions();
	vk::PipelineVertexInputStateCreateInfo vertInputInfo{
		{},
		1, &binding, // vert binding descriptions
		(uint32_t)attribs.size(), attribs.data()  // vert attribs descriptions
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
	rasterizerCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;

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
		//vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};
	vk::PipelineDynamicStateCreateInfo dynStatesCreateInfo{
		{},
		(uint32_t)dynStates.size(), dynStates.data()
	};

	// define and create the pipeline layout
	vector<vk::DescriptorSetLayout> layouts{
		uboLayout, samplerLayout
	};
	vk::PipelineLayoutCreateInfo layoutCreateInfo{
		{},
		(uint32_t)layouts.size(), layouts.data(), // layouts
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
	vk::Pipeline pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCreateInfo);
	return pipeline;
}

void GraphicsVK::CreateFrameBuffers()
{
	for (size_t i = 0; i < imgViews.size(); i++)
	{
		vk::FramebufferCreateInfo fboCreateInfo{
			{},
			renderPass,
			1, &imgViews[i],
			swapInfo.swapExtent.width, swapInfo.swapExtent.height,
			1
		};
		swapchainFrameBuffers.push_back(device.createFramebuffer(fboCreateInfo));
	}
}

void GraphicsVK::CreateCommandResources()
{
	// before we can create command buffers, we need a pool to allocate from
	graphCmdPool = device.createCommandPool({ { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queues.graphicsFamIndex });
	//transfCmdPool = device.createCommandPool({ { vk::CommandPoolCreateFlagBits::eTransient }, queues.transferFamIndex });
	// allocating a cmdBuffer per swapchain FBO
	cmdBuffers = device.allocateCommandBuffers({ graphCmdPool, vk::CommandBufferLevel::ePrimary, (uint32_t)swapchainFrameBuffers.size() });

	// now the secondary command buffers
	// has to be an extra pool cause recording to buffer involves accessing pool, 
	// concurrent use of which is prohibited
	for (int i = 0; i < maxThreads; i++)
	{
		graphSecCmdPools.push_back(device.createCommandPool({ { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queues.graphicsFamIndex }));
		for (int j = 0; j < 3; j++)
			secCmdBuffers[i][j] = device.allocateCommandBuffers({ graphSecCmdPools[i], vk::CommandBufferLevel::eSecondary, (uint32_t)shadersToLoad.size() });
	}

	// we will need semaphores for properly managing the async drawing
	imgAvailable = device.createSemaphore({});
	renderFinished = device.createSemaphore({});

	// our 3 fences to synchronize command submission
	for (size_t i = 0; i < cmdFences.size(); i++)
		cmdFences[i] = device.createFence({ vk::FenceCreateFlagBits::eSignaled });
}

void GraphicsVK::CreateDescriptorPool()
{
	// at the moment only support uniform buffers, but should add img views as well
	array<vk::DescriptorPoolSize, 2> poolSizes{
		vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, Game::maxObjects },
		vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, (uint32_t)texturesToLoad.size() }
	};
	vk::DescriptorPoolCreateInfo poolInfo{
		{}, Game::maxObjects + (uint32_t)texturesToLoad.size(), (uint32_t)poolSizes.size(), poolSizes.data()
	};
	descriptorPool = device.createDescriptorPool(poolInfo);

	// first, the matrices ubo layout
	vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};
	uboLayout = device.createDescriptorSetLayout({ {}, (uint32_t)bindings.size(), bindings.data() });

	// then the samplers
	bindings = {
		vk::DescriptorSetLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
	};
	samplerLayout = device.createDescriptorSetLayout({ {}, (uint32_t)bindings.size(), bindings.data() });
}

void GraphicsVK::CreateDescriptorSet()
{
	// every object will have it's own descriptor set
	array<vk::DescriptorSetLayout, Game::maxObjects> uboLayouts;
	uboLayouts.fill(uboLayout);

	vk::DescriptorSetAllocateInfo info{
		descriptorPool, (uint32_t)uboLayouts.size(), uboLayouts.data()
	};
	uboSets = device.allocateDescriptorSets(info);

	// now we have to update the descriptor's values location
	vector<vk::DescriptorBufferInfo> bufferInfos;
	bufferInfos.reserve(uboSets.size());
	vector<vk::WriteDescriptorSet> writeTargets;
	writeTargets.reserve(uboSets.size());
	for(size_t i=0; i<uboSets.size(); i++)
	{
		// make descriptor point at buffer
		bufferInfos.push_back({
			ubo, GetAlignedOffset(i, sizeof(MatUBO)), sizeof(MatUBO)
		});
		// update descriptor with buffer binding
		writeTargets.push_back({
			uboSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer,
			nullptr, &bufferInfos[i], nullptr
		});
	}
	device.updateDescriptorSets(writeTargets, nullptr);

	// now the samplers
	vector<vk::DescriptorSetLayout> samplerLayouts(texturesToLoad.size(), samplerLayout);
	info = {
		descriptorPool, (uint32_t)samplerLayouts.size(), samplerLayouts.data()
	};
	samplerSets = device.allocateDescriptorSets(info);

	// samplers allocated, need to update them
	vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(texturesToLoad.size());
	writeTargets.clear();
	for (uint32_t i = 0; i < texturesToLoad.size(); i++)
	{
		imageInfos.push_back({
			sampler, textureViews[i], vk::ImageLayout::eShaderReadOnlyOptimal
		});
		writeTargets.push_back({
			samplerSets[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler,
			&imageInfos[i], nullptr, nullptr
		});
	}
	device.updateDescriptorSets(writeTargets, nullptr);
}

void GraphicsVK::CreateUBO()
{
	uboSize = GetAlignedOffset(Game::maxObjects, sizeof(MatUBO));
	CreateBuffer(uboSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, ubo, uboMem);

	mappedUboMem = device.mapMemory(uboMem, 0, uboSize, {});
}

void GraphicsVK::DestroyUBO()
{
	if (mappedUboMem)
	{
		device.unmapMemory(uboMem);
		mappedUboMem = nullptr;
		uboSize = 0;

		device.destroyBuffer(ubo);
		device.freeMemory(uboMem);
	}
}

void GraphicsVK::CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProps, vk::Image &img, vk::DeviceMemory &mem)
{
	vk::ImageCreateInfo imgInfo{
		{}, vk::ImageType::e2D, format, vk::Extent3D {width, height, 1},
		1, 1, vk::SampleCountFlagBits::e1, tiling, usage, 
		vk::SharingMode::eExclusive, 1, nullptr, 
		vk::ImageLayout::ePreinitialized
	};
	img = device.createImage(imgInfo);
	vk::MemoryRequirements reqs = device.getImageMemoryRequirements(img);
	vk::MemoryAllocateInfo memInfo{
		reqs.size, FindMemoryType(reqs.memoryTypeBits, memProps)
	};
	mem = device.allocateMemory(memInfo);
	device.bindImageMemory(img, mem, 0);
}

void GraphicsVK::TransitionLayout(vk::Image img, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	vk::CommandBuffer cmdBuffer = CreateOneTimeCmdBuffer();
	
	vk::ImageMemoryBarrier barrier;
	// transfer from - to
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	// we don't transfer ownership of image
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// the image and it's subresource to perform the transition on
	barrier.image = img;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	// now, depending on layout transitions we need to change access masks
	if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
	}
	else if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	}
	else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::ePreinitialized)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eHostWrite;
	}
	else
		printf("[Error] Transition image layout unsupported: %d -> %d\n", oldLayout, newLayout);

	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
		{},
		0, nullptr,
		0, nullptr,
		1, &barrier);

	EndOneTimeCmdBuffer(cmdBuffer);
}

// images must already be in Transfer Src/Dst Optimal layout
void GraphicsVK::CopyImage(vk::Buffer srcBuff, vk::Image dstImage, uint32_t width, uint32_t height)
{
	vk::CommandBuffer cmdBuffer = CreateOneTimeCmdBuffer();
	
	vk::ImageSubresourceLayers layers{
		vk::ImageAspectFlagBits::eColor, 0, 0, 1
	};

	vk::BufferImageCopy region{
		0, 0, 0, // no offset, tightly packed rows and columns
		layers, {}, { width, height, 1}
	};
	cmdBuffer.copyBufferToImage(srcBuff, dstImage, vk::ImageLayout::eTransferDstOptimal, 
		1, &region);

	EndOneTimeCmdBuffer(cmdBuffer);
}

vk::ImageView GraphicsVK::CreateImageView(vk::Image img, vk::Format format)
{
	vk::ImageSubresourceRange range{
		vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
	};

	vk::ImageViewCreateInfo info{
		{}, img, vk::ImageViewType::e2D, format, vk::ComponentMapping(), range
	};
	return device.createImageView(info);
}

void GraphicsVK::CreateSampler()
{
	vk::SamplerCreateInfo info{
		{},
		vk::Filter::eLinear, vk::Filter::eLinear, // mag, min
		vk::SamplerMipmapMode::eLinear, // sampler mode
		vk::SamplerAddressMode::eRepeat, // u coordinate
		vk::SamplerAddressMode::eRepeat, // v
		vk::SamplerAddressMode::eRepeat, // w
		0, // lod bias
		true, 16, // anisotrophy
		false, vk::CompareOp::eNever, // whether compare the value when sampling it
		0, 0, // min max lod levels
		vk::BorderColor::eIntOpaqueBlack, // color of the border
		false // using normalized coordinates
	};
	sampler = device.createSampler(info);
}

vk::VertexInputBindingDescription GraphicsVK::GetBindingDescription() const
{
	vk::VertexInputBindingDescription desc = {
		0, // binding
		sizeof(Vertex), // stride
		vk::VertexInputRate::eVertex // input rate
	};
	return desc;
}

array<vk::VertexInputAttributeDescription, 3> GraphicsVK::GetAttribDescriptions() const
{
	array<vk::VertexInputAttributeDescription, 3> descs = {
		// vec3 pos
		vk::VertexInputAttributeDescription { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) }, 
		// vec2 uv
		vk::VertexInputAttributeDescription { 1, 0, vk::Format::eR32G32Sfloat,    offsetof(Vertex, uv) }, 
		// vec3 normal
		vk::VertexInputAttributeDescription { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) }, 
	};
	return descs;
}

void GraphicsVK::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProps, vk::Buffer &buff, vk::DeviceMemory &mem)
{
	// create vbo
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	uint32_t famIndices[2] = { queues.graphicsFamIndex, queues.transferFamIndex };
	bufferInfo.queueFamilyIndexCount = 2;
	bufferInfo.pQueueFamilyIndices = famIndices;
	buff = device.createBuffer(bufferInfo);

	// buffer doesn't have any memmory assigned to it yet
	vk::MemoryRequirements reqs = device.getBufferMemoryRequirements(buff);
	vk::MemoryAllocateInfo allocInfo{
		reqs.size,
		FindMemoryType(reqs.memoryTypeBits,	memProps)
	};
	mem = device.allocateMemory(allocInfo);
	// now that we have the memory for it created, we have to bind it to buffer
	device.bindBufferMemory(buff, mem, 0);
}

void GraphicsVK::CopyBuffer(vk::Buffer from, vk::Buffer to, vk::DeviceSize size)
{
	// to copy it over a command buffer is needed
	vk::CommandBuffer cmdBuffer = CreateOneTimeCmdBuffer();
	vk::BufferCopy region{
		0, 0, size
	};
	cmdBuffer.copyBuffer(from, to, 1, &region);
	EndOneTimeCmdBuffer(cmdBuffer);
}

uint32_t GraphicsVK::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags props)
{
	vk::PhysicalDeviceMemoryProperties devProps = physDevice.getMemoryProperties();
	for (uint32_t i = 0; i < devProps.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (devProps.memoryTypes[i].propertyFlags & props) == props)
			return i;
	}
	return -1;
}

vk::CommandBuffer GraphicsVK::CreateOneTimeCmdBuffer()
{
	vk::CommandBufferAllocateInfo info{
		graphCmdPool, vk::CommandBufferLevel::ePrimary, 1
	};
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(info)[0];

	vk::CommandBufferBeginInfo beginInfo{
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr
	};
	cmdBuffer.begin(beginInfo);
	return cmdBuffer;
}

void GraphicsVK::EndOneTimeCmdBuffer(vk::CommandBuffer cmdBuff)
{
	cmdBuff.end();
	
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;
	queues.graphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE);
	queues.graphicsQueue.waitIdle();
	
	device.freeCommandBuffers(graphCmdPool, 1, &cmdBuff);
}

bool GraphicsVK::LayersAvailable(const vector<const char*> &validationLayers)
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

VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsVK::DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData)
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