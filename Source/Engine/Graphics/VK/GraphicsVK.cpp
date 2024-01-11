#include "Precomp.h"

#ifdef USE_VULKAN

#include "GraphicsVK.h"
#include "Camera.h"
#include "Game.h"
#include "Terrain.h"
#include "VisualObject.h"

const vector<const char *> GraphicsVK::ourRequiredLayers = {
#ifdef _DEBUG
	"VK_LAYER_LUNARG_standard_validation",
#endif
	"VK_LAYER_LUNARG_monitor"
};
const vector<const char *> GraphicsVK::ourRequiredExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

GraphicsVK::GraphicsVK()
	: Graphics()
	, myMaxThreads(1)
	, myThreadCounter(0)
	, mySlotIndex(0)
	, myIsPaused(false)
	, myGatherStarted(false)
	, mySwapchain(VK_NULL_HANDLE)
	, myMappedUboMem(nullptr)
{
}

// Public Methods
void GraphicsVK::Init(const vector<Terrain*>& aTerrainList)
{
	if (glfwVulkanSupported() == GLFW_FALSE)
	{
		std::println("[Error] Vulkan loader unavailable");
		return;
	}

	ourActiveGraphics = this;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	myWindow = glfwCreateWindow(ourWidth, ourHeight, "VEngine - VK", nullptr, nullptr);
	glfwSetInputMode(myWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowSizeCallback(myWindow, OnWindowResized);

	CreateInstance();
	CreateSurface();
	CreateDevice();
	CreateSwapchain();
	CreateRenderPass();
	CreateDepthTexture();
	CreateFrameBuffers();
	CreateCommandResources();
	CreateUBO();
	CreateDescriptorPool();
	LoadResources(aTerrainList);
	CreateSampler();
	CreateDescriptorSet();

	ResetRenderCalls();
}

void GraphicsVK::LoadResources(const vector<Terrain*>& aTerrainList)
{
	// same order as GraphicsGL
	// shaders
	// define and create the only pipeline layout we use
	vector<vk::DescriptorSetLayout> layouts{
		myUboLayout, mySamplerLayout
	};
	vk::PipelineLayoutCreateInfo layoutCreateInfo{
		{},
		(uint32_t)layouts.size(), layouts.data(), // layouts
		0, nullptr  // push constant ranges
	};
	myPipelineLayout = myDevice.createPipelineLayout(layoutCreateInfo);

	// Actual pipelines
	for (string shaderName : ourShadersToLoad)
	{
		vk::Pipeline pipeline = CreatePipeline(shaderName);
		myPipelines.push_back(pipeline);
	}

	// models
	{
		vector<Vertex> vertices;
		vector<IndexType> indices;

		// have to perform vert and index offset calculations as well
		for (string modelName : ourModelsToLoad)
		{
			Model m;
			m.myIndexOffset = indices.size();
			
			if (modelName.substr(0, 2) == "%t")
			{
				m.myVertexOffset = vertices.size();

				int index = stoi(modelName.substr(2), nullptr);
				const Terrain& t = *aTerrainList[index];
				vertices.insert(vertices.end(), t.GetVertBegin(), t.GetVertEnd());
				indices.insert(indices.end(), t.GetIndBegin(), t.GetIndEnd());
				m.myCenter = t.GetCenter();
				m.mySphereRadius = t.GetRange();
			}
			else
			{
				m.myVertexOffset = 0;

				LoadModel(modelName, vertices, indices, m.myCenter, m.mySphereRadius);
			}

			m.myVertexCount = vertices.size() - m.myVertexOffset;
			m.myIndexCount = indices.size() - m.myIndexOffset;

			myModels.push_back(m);
		}

		// creating staging vbo
		vk::Buffer stagingBuff;
		vk::DeviceMemory stagingMem;
		vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc;
		vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		vk::DeviceSize size = sizeof(Vertex) * vertices.size();
		CreateBuffer(size, usage, memProps, stagingBuff, stagingMem);

		// now buffer the memory to staging
		{
			void* data = myDevice.mapMemory(stagingMem, 0, size, {});
			memcpy(data, vertices.data(), size);
			myDevice.unmapMemory(stagingMem);
		}

		// creating vbo myDevice local
		usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
		CreateBuffer(size, usage, memProps, myVbo, myVboMem);

		// create the command buffer to perform cross-buffer copy
		CopyBuffer(stagingBuff, myVbo, size);

		// cleanup
		myDevice.destroyBuffer(stagingBuff);
		myDevice.freeMemory(stagingMem);

		// same for ibo
		usage = vk::BufferUsageFlagBits::eTransferSrc;
		memProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		size = sizeof(IndexType) * indices.size();
		CreateBuffer(size, usage, memProps, stagingBuff, stagingMem);

		// now buffer the memory to staging
		{
			void* data = myDevice.mapMemory(stagingMem, 0, size, {});
			memcpy(data, indices.data(), size);
			myDevice.unmapMemory(stagingMem);
		}

		// creating ibo myDevice local
		usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
		CreateBuffer(size, usage, memProps, myIbo, myIboMem);

		// transfer
		CopyBuffer(stagingBuff, myIbo, size);

		// cleanup
		myDevice.destroyBuffer(stagingBuff);
		myDevice.freeMemory(stagingMem);
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
		myTextures.reserve(ourTexturesToLoad.size());
		myTextureMems.reserve(ourTexturesToLoad.size());
		for (string textureName : ourTexturesToLoad)
		{
			// load up the texture data
			int width, height, channels;
			unsigned char *pixels = LoadTexture("assets/textures/" + textureName, &width, &height, &channels, STBI_rgb_alpha);
			vk::DeviceSize texSize = height * width * 4;

			// copy it over to buffer
			uint8_t *data = (uint8_t*)myDevice.mapMemory(stagingMem, 0, stagingSize, {});
				memcpy(data, pixels, (size_t)texSize);
			myDevice.unmapMemory(stagingMem);

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
			myTextures.push_back(text);
			myTextureMems.push_back(textMem);
			FreeTexture(pixels);

			// create a textureView for it
			vk::ImageView view = CreateImageView(text, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
			myTextureViews.push_back(view);
		}

		myDevice.destroyBuffer(stagingBuff);
		myDevice.freeMemory(stagingMem);
	}
}

void GraphicsVK::BeginGather()
{
	// reset the thread infos in case another thread spooled up
	myThreadLocalIndices.clear();
	myThreadCounter = 0;

	// acquire image to render to
	myCurrentImageIndex = myDevice.acquireNextImageKHR(mySwapchain, UINT32_MAX, myImgAvailable, VK_NULL_HANDLE).value;

	// before we start recording the command buffer, need to make sure that it has finished being executed
	// TODO: revisit to refactor RWBuffer usage
	myCmdFences.Advance();
	const vk::Fence& fence = myCmdFences.GetRead();
	myDevice.waitForFences(1, &fence, false, ~0ull);
	myDevice.resetFences(1, &fence);

	// advance our buffer, since it has finished being used
	mySlotIndex = 0;
	myCmdBuffers.Advance();
	mySecCmdBuffers.Advance();
	myUboSets.Advance();
	myMappedUboMem.Advance();

	// color and depth clear vals
	array<vk::ClearValue, 2> clearVals;
	array<uint32_t, 4> clearCol = { 0, 0, 0, 1 };
	clearVals[0] = vk::ClearValue(vk::ClearColorValue(clearCol));
	clearVals[1] = vk::ClearValue({ 1, 0 });

	// begin recording to command buffer
	const vk::CommandBuffer& primaryCmdBuffer = myCmdBuffers.GetRead();
	primaryCmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eSimultaneousUse });

	// first we render all geometry - start the render pass
	vk::RenderPassBeginInfo beginInfo{
		myRenderPass,
		mySwapchainFrameBuffers[myCurrentImageIndex],
		{ { 0, 0 }, mySwapInfo.mySwapExtent },
		(uint32_t)clearVals.size(), clearVals.data()
	};
	primaryCmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

	// have to begin all secondary buffers here
	const PerThreadCmdBuffers& perThreadBuffers = mySecCmdBuffers.GetRead();
	for(const PerPipelineCmdBuffers& perPipelineBuffers : perThreadBuffers)
	{
		for(size_t pipeline=0, length = perPipelineBuffers.size(); pipeline < length; pipeline++)
		{
			const vk::CommandBuffer& buffer = perPipelineBuffers[pipeline];
			// need inheritance info since secondary buffer
			vk::CommandBufferInheritanceInfo inheritanceInfo{
				myRenderPass, 0, // render using subpass#0 of renderpass
				mySwapchainFrameBuffers[myCurrentImageIndex],
				0
			};
			vk::CommandBufferBeginInfo info{
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit 
					| vk::CommandBufferUsageFlagBits::eRenderPassContinue
					| vk::CommandBufferUsageFlagBits::eSimultaneousUse,
				&inheritanceInfo
			};
			buffer.begin(info);

			// bind the vbo and ibo
			vk::DeviceSize offset = 0;
			buffer.bindVertexBuffers(0, 1, &myVbo, &offset);
			buffer.bindIndexBuffer(myIbo, 0, vk::IndexType::eUint32);

			// bind the pipeline for rendering with
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, myPipelines[pipeline]);

			// update the dynamic states
			vk::Viewport viewport{
				0, 0,
				(float)mySwapInfo.mySwapExtent.width, (float)mySwapInfo.mySwapExtent.height,
				0, 1
			};
			buffer.setViewport(0, 1, &viewport);

			vk::Rect2D scissor{ {}, mySwapInfo.mySwapExtent };
			buffer.setScissor(0, 1, &scissor);
		}
	}
	myGatherStarted = true;
}

void GraphicsVK::Render(const Camera& aCam, const GameObject* aGO)
{
	if (myIsPaused)
	{
		return;
	}

	const Renderer* r = aGO->GetRenderer();
	if (!r)
	{
		return;
	}

	bool exists = false;
	uint& threadId = myThreadLocalIndices.local(exists);
	if(!exists)
	{
		tbb::spin_mutex::scoped_lock lock(myThreadCounterSpinlock);
		threadId = myThreadCounter++;
		ASSERT_STR(threadId < myMaxThreads, "Unaccounted thread appeared, can't find thread-local info for it");
	}

	// get the vector of secondary buffers for thread
	const PerPipelineCmdBuffers& buffers = mySecCmdBuffers.GetRead()[threadId];
	// we need to find the corresponding secondary buffer
	ShaderId pipelineInd = r->GetShader();
	Model m = myModels[r->GetModel()];
	size_t index = 0;
	{
		tbb::spin_mutex::scoped_lock lock(mySlotIndexMutex);
		index = mySlotIndex++;
		ASSERT_STR(index < Game::maxObjects, "Managed to exceed memory capacity for objects!");
		myRenderCalls++;
	}

	// update the uniforms
	MatUBO matrices;
	matrices.myModel = aGO->GetMatrix();
	matrices.myMvp = aCam.Get() * matrices.myModel;
	memcpy((char*)myMappedUboMem.GetRead() + GetAlignedOffset(index, sizeof(MatUBO)), &matrices, sizeof(MatUBO));
	
	// draw out all the indices
	array<vk::DescriptorSet, 2> setsToBind{
		myUboSets.GetRead()[index], mySamplerSets[r->GetTexture()]
	};
	buffers[pipelineInd].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, myPipelineLayout, 
		0, (uint32_t)setsToBind.size(), setsToBind.data(), 0, nullptr);
	buffers[pipelineInd].drawIndexed(static_cast<uint32_t>(m.myIndexCount), 1, static_cast<uint32_t>(m.myIndexOffset), static_cast<int32_t>(m.myVertexOffset), 0);
}

void GraphicsVK::Display()
{
	// early exit if no gather active - helps with resizing
	if (!myGatherStarted)
	{
		return;
	}

	// before we can execute them, have to end them
	const PerThreadCmdBuffers& perThreadBuffers = mySecCmdBuffers.GetRead();
	for (const PerPipelineCmdBuffers& threadBuffersPair : perThreadBuffers)
	{
		for (const vk::CommandBuffer& perPipelineBuffers : threadBuffersPair)
		{
			perPipelineBuffers.end();
		}
	}

	// draw out all the accumulated render calls
	const vk::CommandBuffer& primaryCmdBuffer = myCmdBuffers.GetRead();
	{
		const PerThreadCmdBuffers& perThreadBuffers = mySecCmdBuffers.GetRead();
		// we know how many there are total - every thread has per-pipeline command buffers
		const size_t totalBuffers = perThreadBuffers.size() * myPipelines.size();
		vk::CommandBuffer* allBuffers = new vk::CommandBuffer[totalBuffers];
		// rearrange them to avoid pipeline changes - they might still happen, but if they do,
		// fixing it will be faster since this will accomodate it. 
		// TODO: check if pipeline switching still happens in the driver
		// TODO: Though the following code is not cache-friendly, not going to change it just yet 
		// because it's should have a small amount of iterations for now (only 3 pipelines)
		size_t currentSlot = 0;
		for (size_t pipeline = 0, pipelinesLength = myPipelines.size(); pipeline < pipelinesLength; pipeline++)
		{
			for (uint32_t thread = 0; thread < myMaxThreads; thread++)
			{
				allBuffers[currentSlot++] = perThreadBuffers[thread][pipeline];
			}
		}

		// submit them all
		primaryCmdBuffer.executeCommands(static_cast<uint32_t>(totalBuffers), allBuffers);
	}

	// finish up the render pass
	primaryCmdBuffer.endRenderPass();
	// finish the recording
	primaryCmdBuffer.end();

	// submit the queue
	// make sure it waits for the image to become available before we start outputting color
	vk::PipelineStageFlags waitAt = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submitInfo{
		1, &myImgAvailable, &waitAt,
		1, &primaryCmdBuffer,
		1, &myRenderFinished
	};
	myQueues.myGraphicsQueue.submit(1, &submitInfo, myCmdFences.GetRead());

	// present the results of the drawing
	vk::PresentInfoKHR presentInfo{
		1, &myRenderFinished,
		1, &mySwapchain, &myCurrentImageIndex,
		nullptr
	};
	myQueues.myGraphicsQueue.presentKHR(presentInfo);
	myGatherStarted = false;
}

void GraphicsVK::CleanUp()
{
	// gonna make sure all the tasks are finished before we can start destroying resources
	myDevice.waitIdle();

	myDevice.destroySampler(mySampler);

	for (vk::ImageView v : myTextureViews)
	{
		myDevice.destroyImageView(v);
	}
	myTextureViews.clear();
	for (vk::Image t : myTextures)
	{
		myDevice.destroyImage(t);
	}
	myTextures.clear();
	for (vk::DeviceMemory m : myTextureMems)
	{
		myDevice.freeMemory(m);
	}
	myTextureMems.clear();

	myDevice.destroyDescriptorPool(myDescriptorPool);

	DestroyUBO();
	myDevice.destroyDescriptorSetLayout(myUboLayout);
	myDevice.destroyDescriptorSetLayout(mySamplerLayout);

	myDevice.destroyImageView(myDepthImgView);
	myDevice.destroyImage(myDepthImg);
	myDevice.freeMemory(myDepthImgMem);

	for (vk::Fence fence : myCmdFences)
	{
		myDevice.destroyFence(fence);
	}
	myDevice.destroyBuffer(myVbo);
	myDevice.freeMemory(myVboMem);
	myDevice.destroyBuffer(myIbo);
	myDevice.freeMemory(myIboMem);

	myDevice.destroySemaphore(myImgAvailable);
	myDevice.destroySemaphore(myRenderFinished);
	myDevice.destroyCommandPool(myGraphCmdPool);
	//myDevice.destroyCommandPool(transfCmdPool);
	for (vk::CommandPool pool : myGraphSecCmdPools)
	{
		myDevice.destroyCommandPool(pool);
	}
	myGraphSecCmdPools.clear();
	
	for (vk::Framebuffer b : mySwapchainFrameBuffers)
	{
		myDevice.destroyFramebuffer(b);
	}
	mySwapchainFrameBuffers.clear();
	
	for (vk::Pipeline pipeline : myPipelines)
	{
		myDevice.destroyPipeline(pipeline);
	}
	myPipelines.clear();
	
	myDevice.destroyPipelineLayout(myPipelineLayout);
	myDevice.destroyRenderPass(myRenderPass);

	for (vk::ShaderModule shaderMod : myShaderModules)
	{
		myDevice.destroyShaderModule(shaderMod);
	}
	myShaderModules.clear();

	for (vk::ImageView v : myImgViews)
	{
		myDevice.destroyImageView(v);
	}
	myImgViews.clear();
	myDevice.destroySwapchainKHR(mySwapchain);
	myInstance.destroySurfaceKHR(mySurface);

	myDevice.destroy();
#ifdef _DEBUG
	if (DestroyDebugReportCallback != nullptr)
	{
		DestroyDebugReportCallback(myInstance, myDebugCallback, nullptr);
	}
#endif
	myInstance.destroy();

	glfwDestroyWindow(myWindow);
	ourActiveGraphics = nullptr;
}

void GraphicsVK::SetMaxThreads(uint32_t aMaxThreadCount)
{
	myMaxThreads = aMaxThreadCount;
}

void GraphicsVK::OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight)
{
	if (aWidth == 0 && aHeight == 0)
	{
		return;
	}

	((GraphicsVK*)ourActiveGraphics)->WindowResized(aWidth, aHeight);
}

void GraphicsVK::WindowResized(int aWidth, int aHeight)
{
	myIsPaused = true;
	// gonna make sure all the tasks are finished before we can start destroying resources
	ourWidth = aWidth;
	ourHeight = aHeight;

	// force the end of rendering
	Display();
	myDevice.waitIdle();
	
	// destroy first
	// framebuffers of swapchain
	for (vk::Framebuffer b : mySwapchainFrameBuffers)
	{
		myDevice.destroyFramebuffer(b);
	}
	mySwapchainFrameBuffers.clear();

	// depth texture
	myDevice.destroyImageView(myDepthImgView);
	myDevice.destroyImage(myDepthImg);
	myDevice.freeMemory(myDepthImgMem);

	// swapchain images
	for (vk::ImageView v : myImgViews)
	{
		myDevice.destroyImageView(v);
	}
	myImgViews.clear();
	myDevice.destroySwapchainKHR(mySwapchain);

	// then recreate
	CreateSwapchain();
	CreateDepthTexture();
	CreateFrameBuffers();

	myIsPaused = false;
}

// Private Methods
void GraphicsVK::CreateInstance()
{
	// creating the instance
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "VulkanRTSEngine";
	appInfo.pEngineName = "VEngine";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	vk::InstanceCreateInfo instInfo;
	instInfo.pApplicationInfo = &appInfo;
	// first, the extensions
	vector<const char *> extensions;
	uint32_t extensionCount;
	const char **baseExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	for (uint32_t i = 0; i < extensionCount; i++)
	{
		extensions.push_back(baseExtensions[i]);
	}

#ifdef _DEBUG
	if (LayersAvailable(ourRequiredLayers))
	{
		instInfo.enabledLayerCount = static_cast<uint32_t>(ourRequiredLayers.size());
		instInfo.ppEnabledLayerNames = ourRequiredLayers.data();

		// in order to receive the messages, we need a callback extension
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
#endif

	instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instInfo.ppEnabledExtensionNames = extensions.data();

	myInstance = vk::createInstance(instInfo);

#ifdef _DEBUG
	CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)myInstance.getProcAddr("vkCreateDebugReportCallbackEXT");
	DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)myInstance.getProcAddr("vkDestroyDebugReportCallbackEXT");

	if (CreateDebugReportCallback != nullptr)
	{
		VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
		callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		callbackCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)DebugCallback;

		CreateDebugReportCallback(myInstance, &callbackCreateInfo, nullptr, &myDebugCallback);
	}
#endif
}

void GraphicsVK::CreateDevice()
{
	// first we gotta find our physical myDevice before we can create a logical one
	vector<vk::PhysicalDevice> devices = myInstance.enumeratePhysicalDevices();
	if (devices.size() == 0)
	{
		std::println("[Error] No vulkan devices found!");
		return;
	}

	std::println("[Info] Found {} vulkan devices", devices.size());
	vk::PhysicalDevice pickedDevice = VK_NULL_HANDLE;
	for (vk::PhysicalDevice myDevice : devices)
	{
		if (IsSuitable(myDevice))
		{
			pickedDevice = myDevice;
			break;
		}
	}
	if (!pickedDevice)
	{
		std::println("[Error] No suitable vulkan myDevice found!");
		return;
	}
	myPhysDevice = pickedDevice;

	// then we gotta pick queue families for our use (graphics, compute and transfer)
	vector<vk::QueueFamilyProperties> queueFamProps = myPhysDevice.getQueueFamilyProperties();
	int i = 0;
	
	// first going to attemp to find specialized queues
	// this way we can better utilize gpu
	set<uint32_t> used;
	for (vk::QueueFamilyProperties props : queueFamProps)
	{
		if (props.queueFlags == vk::QueueFlagBits::eGraphics)
		{
			myQueues.myGraphicsFamIndex = i;
			used.insert(i);
		}
		else if (props.queueFlags == vk::QueueFlagBits::eCompute)
		{
			myQueues.myComputeFamIndex = i;
			used.insert(i);
		}
		else if (props.queueFlags == vk::QueueFlagBits::eTransfer)
		{
			myQueues.myTransferFamIndex = i;
			used.insert(i);
		}
		if (pickedDevice.getSurfaceSupportKHR(i, mySurface))
		{
			myQueues.myPresentFamIndex = i;
		}
		i++;
	}
	// this time, if we have something not set find a suitable generic family
	// additionally, if multiple fitting families are found, try to use different ones
	i = 0;
	for (vk::QueueFamilyProperties props : queueFamProps)
	{
		if (props.queueFlags & vk::QueueFlagBits::eGraphics 
			&& (myQueues.myGraphicsFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			myQueues.myGraphicsFamIndex = i;
			used.insert(i);
		}
		if (props.queueFlags & vk::QueueFlagBits::eCompute 
			&& (myQueues.myComputeFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			myQueues.myComputeFamIndex = i;
			used.insert(i);
		}
		if (props.queueFlags & vk::QueueFlagBits::eTransfer 
			&& (myQueues.myTransferFamIndex == UINT32_MAX || find(used.cbegin(), used.cend(), i) == used.cend()))
		{
			myQueues.myTransferFamIndex = i;
			used.insert(i);
		}
		i++;
	}
	std::println("[Info] Using queue families: graphics={}, compute={} and transfer={}", myQueues.myGraphicsFamIndex, myQueues.myComputeFamIndex, myQueues.myTransferFamIndex);

	// proper set-up of queues is a bit convoluted, cause it has to take care of the 3! cases of combinations
	vector<vk::DeviceQueueCreateInfo> queuesToCreate;
	// graphics queue should always be present
	const float priority = 1; // for simplicity's sake, priority is always the same
	for (uint32_t fam : used)
	{
		queuesToCreate.push_back({ {}, fam, 1, &priority });
	}
	
	std::println("[Info] Creating {} queues", used.size());

	// now, finally the logical myDevice creation
	vk::PhysicalDeviceFeatures features;
	vk::DeviceCreateInfo devCreateInfo{
		{},
		(uint32_t)queuesToCreate.size(), queuesToCreate.data(),
		(uint32_t)ourRequiredLayers.size(), ourRequiredLayers.data(),
		(uint32_t)ourRequiredExtensions.size(), ourRequiredExtensions.data(),
		&features
	};
	myDevice = myPhysDevice.createDevice(devCreateInfo);

	// queues are auto-created on myDevice creation, so just have to get their handles
	// shared queues will deal with parallelization with myDevice events
	myQueues.myGraphicsQueue = myDevice.getQueue(myQueues.myGraphicsFamIndex, 0);
	//queues.myComputeQueue = myDevice.getQueue(queues.myComputeFamIndex, 0);
	//queues.myTransferQueue = myDevice.getQueue(queues.myTransferFamIndex, 0);
	myQueues.myPresentQueue = myDevice.getQueue(myQueues.myPresentFamIndex, 0);

	// we need the alignment for our uniform buffers
	myLimits = myPhysDevice.getProperties().limits;

	// now that we have the myDevice, we can find what depth format we'll use
	myDepthFormat = FindSupportedFormat(
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);

	// before we can create command buffers, we need a pool to allocate from
	myGraphCmdPool = myDevice.createCommandPool({ { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, myQueues.myGraphicsFamIndex });
	// TODO: look into using transfer command pool
	//transfCmdPool = myDevice.createCommandPool({ { vk::CommandPoolCreateFlagBits::eTransient }, queues.myTransferFamIndex });
}

// TODO: extend this to use proper checking of caps and initialization of available resources
// right now it just checks for a device with all queues
bool GraphicsVK::IsSuitable(const vk::PhysicalDevice& aDevice)
{
	// currently support only Discrete/Integrated GPUs
	{
		vk::PhysicalDeviceProperties props = aDevice.getProperties();
		bool foundDeviceType = props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
			|| props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
		if (!foundDeviceType)
		{
			return false;
		}
	}

	// we need graphics, compute, transfer and present (can be same queue family)
	{
		bool foundQueueFamilies = true;
		vector<vk::QueueFamilyProperties> queueFamProps = aDevice.getQueueFamilyProperties();
		bool foundQueues[4] = { false, false, false, false };
		uint32_t i = 0;
		for (vk::QueueFamilyProperties prop : queueFamProps)
		{
			// source of Flags<...> has a funky implementation of operator bool():
			// explicit operator bool() const { return !!m_mask; }
			foundQueues[0] |= bool(prop.queueFlags & vk::QueueFlagBits::eGraphics);
			foundQueues[1] |= bool(prop.queueFlags & vk::QueueFlagBits::eCompute);
			foundQueues[2] |= bool(prop.queueFlags & vk::QueueFlagBits::eTransfer);
			foundQueues[3] |= aDevice.getSurfaceSupportKHR(i, mySurface) == VK_TRUE;
			i++;
		}
		for (int i = 0; i < 4; i++)
		{
			foundQueueFamilies &= foundQueues[i];
		}

		if (!foundQueueFamilies)
		{
			return false;
		}
	}

	// we also need the requested extensions
	{
		set<string> reqExts(ourRequiredExtensions.cbegin(), ourRequiredExtensions.cend()); // has to be string for the == to work
		vector<vk::ExtensionProperties> supportedExts = aDevice.enumerateDeviceExtensionProperties();
		for (vk::ExtensionProperties ext : supportedExts)
		{
			reqExts.erase(ext.extensionName);
		}
		if (!reqExts.empty())
		{
			return false;
		}
	}

	// though it's strange to have swapchain support but no proper formats/present modes
	// have to check just to be safe - it's not stated in the api spec that it's always > 0
	{
		vector<vk::SurfaceFormatKHR> mySupportedFormats = aDevice.getSurfaceFormatsKHR(mySurface);
		vector<vk::PresentModeKHR> presentModes = aDevice.getSurfacePresentModesKHR(mySurface);
		if (mySupportedFormats.empty() || presentModes.empty())
		{
			return false;
		}
	}

	return true;
}

void GraphicsVK::CreateSurface()
{
	// another vulkan.hpp issue - can't get underlying pointer, have to manually construct surface
	VkSurfaceKHR vkSurface;
	if (glfwCreateWindowSurface(myInstance, myWindow, nullptr, &vkSurface) != VK_SUCCESS)
	{
		std::println("[Error] Surface creation failed");
		return;
	}
	// good thing is we can still keep the vulkan.hpp definitions by manually constructing them
	mySurface = vk::SurfaceKHR(vkSurface);
}

void GraphicsVK::CreateSwapchain()
{
	// we found a proper myDevice and surface, fetch the swapchain caps
	mySwapInfo.mySurfCaps = myPhysDevice.getSurfaceCapabilitiesKHR(mySurface);
	mySwapInfo.mySupportedFormats = myPhysDevice.getSurfaceFormatsKHR(mySurface);
	mySwapInfo.myPresentModes = myPhysDevice.getSurfacePresentModesKHR(mySurface);

	// by this point we have the capabilities of the surface queried, we just have to pick one
	// myDevice might be able to support all the formats, so check if it's the case
	vk::SurfaceFormatKHR format;
	if (mySwapInfo.mySupportedFormats.size() == 1 
		&& mySwapInfo.mySupportedFormats[0].format == vk::Format::eUndefined)
	{
		format = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}
	else 
	{
		// myDevice only selects a certain set of formats, try to find ours
		bool wasFound = false;
		for (vk::SurfaceFormatKHR f : mySwapInfo.mySupportedFormats)
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
		{
			format = mySwapInfo.mySupportedFormats[0];
		}
	}
	mySwapInfo.myImgFormat = format.format;

	// now to figure out the present mode. mailbox is the best(vsync off), then fifo(on), then immediate(off)
	// only fifo is guaranteed to be present on all devices
	vk::PresentModeKHR mode = vk::PresentModeKHR::eFifo;
	for (vk::PresentModeKHR m : mySwapInfo.myPresentModes)
	{
		if (m == vk::PresentModeKHR::eMailbox)
		{
			mode = m;
			break;
		}
	}

	// lastly (almost), need to find the swap extent(swapchain image resolutions)
	// vulkan myWindow surface may provide us with the ability to set our own extents
	if (mySwapInfo.mySurfCaps.currentExtent.width == UINT32_MAX)
	{
		uint32_t w = glm::clamp((uint32_t)ourWidth, mySwapInfo.mySurfCaps.minImageExtent.width,  mySwapInfo.mySurfCaps.maxImageExtent.width);
		uint32_t h = glm::clamp((uint32_t)ourHeight, mySwapInfo.mySurfCaps.minImageExtent.height, mySwapInfo.mySurfCaps.maxImageExtent.height);
		mySwapInfo.mySwapExtent = { w, h };
	}
	else
	{
		// or it might not and we have to use it's provided extent
		mySwapInfo.mySwapExtent = mySwapInfo.mySurfCaps.currentExtent;
	}

	// the actual last step is the determination of the image count in the swapchain
	// we want tripple buffering, but have to make sure that it's actually supported
	uint32_t imageCount = 3;
	// checking if we're in range of the caps
	if (imageCount < mySwapInfo.mySurfCaps.minImageCount)
	{
		// maybe it supports minimum 4 (strange, but have to account for strange)
		imageCount = mySwapInfo.mySurfCaps.minImageCount; 
	}
	else if (mySwapInfo.mySurfCaps.maxImageCount > 0 
			&& imageCount > mySwapInfo.mySurfCaps.maxImageCount) 
	{
		// 0 means only bound by myDevice memory
		imageCount = mySwapInfo.mySurfCaps.maxImageCount;
	}
		
	std::println("[Info] Swapchain params: format={}, colorSpace={}, presentMode={}, extent={{{},{}}}, imgCount={}",
		mySwapInfo.myImgFormat, format.colorSpace, mode, mySwapInfo.mySwapExtent.width, mySwapInfo.mySwapExtent.height, imageCount);

	// time to fill up the swapchain creation information
	vk::SwapchainCreateInfoKHR swapCreateInfo;
	swapCreateInfo.surface = mySurface;
	swapCreateInfo.minImageCount = imageCount;
	swapCreateInfo.imageFormat = mySwapInfo.myImgFormat;
	swapCreateInfo.imageColorSpace = format.colorSpace;
	swapCreateInfo.imageExtent = mySwapInfo.mySwapExtent;
	swapCreateInfo.imageArrayLayers = 1;
	swapCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; // change this later to eTransferDst to enable image-to-image copy op
	// because present family might be different from graphics family we might need to share the images
	if (myQueues.myGraphicsFamIndex != myQueues.myPresentFamIndex)
	{
		uint32_t indices[] = { myQueues.myGraphicsFamIndex, myQueues.myPresentFamIndex };
		swapCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapCreateInfo.queueFamilyIndexCount = 2;
		swapCreateInfo.pQueueFamilyIndices = indices;
	}
	else
	{
		swapCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}
	swapCreateInfo.presentMode = mode;
	swapCreateInfo.clipped = VK_TRUE;
	// TODO: Investigate - for some reason I get validation errors by passing swapchain here
	swapCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	mySwapchain = myDevice.createSwapchainKHR(swapCreateInfo);

	// swapchain creates the images for us to render to
	myImages = myDevice.getSwapchainImagesKHR(mySwapchain);
	std::println("[Info] Images acquired: {}", myImages.size());

	// but in order to use them, we need imageviews
	for (int i = 0; i < myImages.size(); i++)
	{
		myImgViews.push_back(CreateImageView(myImages[i], mySwapInfo.myImgFormat, vk::ImageAspectFlagBits::eColor));
	}
}

void GraphicsVK::CreateRenderPass()
{
	// in order to define a render pass, we need to collect all attachments for it
	// as of now we only render color to single texture and depth
	vk::AttachmentDescription colorAttachment{
		{},
		mySwapInfo.myImgFormat, vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // load/store
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil load/store
		vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR // initial/final layouts
	};
	vk::AttachmentDescription depthAttachment{
		{},
		myDepthFormat, vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal
	};
	// in order to reference what attachment will be referenced in a subpass, attachment reference needs to be created
	vk::AttachmentReference colorRef{
		0, vk::ImageLayout::eColorAttachmentOptimal
	};
	vk::AttachmentReference depthRef{
		1, vk::ImageLayout::eDepthStencilAttachmentOptimal
	};
	// create the only subpass we need for color rendering
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;

	// specifying dependencies to tell Vulkan api how to transition images
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; // we only have 1 subpass, so it's 0

	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};

	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

	// now that we have all the required info, create the render pass
	array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	vk::RenderPassCreateInfo passCreateInfo{
		{},
		(uint32_t)attachments.size(), attachments.data(), // attachments
		1, &subpass, // subpasses
		1, &dependency // dependencies
	};
	myRenderPass = myDevice.createRenderPass(passCreateInfo);
}

vk::Pipeline GraphicsVK::CreatePipeline(string aName)
{
	// first, getting the shaders
	string frag = ReadFile("assets/VulkanShaders/" + aName + "-frag.spv");
	string vert = ReadFile("assets/VulkanShaders/" + aName + "-vert.spv");

	// and wrapping them in to something we can pass around vulkan apis
	vk::ShaderModule vertShader = myDevice.createShaderModule({ {}, vert.size(), (const uint32_t*)vert.data() });
	vk::ShaderModule fragShader = myDevice.createShaderModule({ {}, frag.size(), (const uint32_t*)frag.data() });

	myShaderModules.push_back(vertShader);
	myShaderModules.push_back(fragShader);

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
		(float)mySwapInfo.mySwapExtent.width, (float)mySwapInfo.mySwapExtent.height,
		0, 1
	};

	// scissors ...
	vk::Rect2D scissors{
		{0, 0},
		mySwapInfo.mySwapExtent
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
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth,
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynStatesCreateInfo{
		{},
		(uint32_t)dynStates.size(), dynStates.data()
	};

	// enable depth testing
	vk::PipelineDepthStencilStateCreateInfo depthInfo{
		{}, 
		true, true, // depth test and depth write
		vk::CompareOp::eLess, // depth test op
		false, false, // depth bounds and stencil tests
		vk::StencilOpState(), vk::StencilOpState(), // stencil's front and back tests
		0, 1 // depth bounds
	};

	// we have the layour and the renderpass - all that's needed for an actual pipeline
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
		{},
		(uint32_t)stages.size(), stages.data(),
		&vertInputInfo, &inputAssemblyInfo,
		nullptr, &viewportCreateInfo,
		&rasterizerCreateInfo, &multisampleCreateInfo,
		&depthInfo, &colorBlendCreateInfo,
		&dynStatesCreateInfo, myPipelineLayout,
		myRenderPass, 0,
		VK_NULL_HANDLE, -1
	};
	vk::Pipeline pipeline = myDevice.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCreateInfo);
	return pipeline;
}

void GraphicsVK::CreateFrameBuffers()
{
	for (size_t i = 0; i < myImgViews.size(); i++)
	{
		// each render target will use the same depth image internally
		array<vk::ImageView, 2> views = { myImgViews[i], myDepthImgView };
		vk::FramebufferCreateInfo fboCreateInfo {
			{},
			myRenderPass,
			(uint32_t)views.size(), views.data(),
			mySwapInfo.mySwapExtent.width, mySwapInfo.mySwapExtent.height,
			1
		};
		mySwapchainFrameBuffers.push_back(myDevice.createFramebuffer(fboCreateInfo));
	}
}

void GraphicsVK::CreateCommandResources()
{
	{
		// allocating a cmdBuffer per swapchain FBO
		vector<vk::CommandBuffer> buffers = myDevice.allocateCommandBuffers({ myGraphCmdPool, vk::CommandBufferLevel::ePrimary, 3 });
		size_t i = 0;
		for (vk::CommandBuffer& cmdBuffer : myCmdBuffers)
		{
			cmdBuffer = buffers[i++];
		}
	}

	{
		// now the secondary command buffers
		// has to be an extra pool cause recording to buffer involves accessing pool, 
		// concurrent use of which is prohibited
		for (uint32_t thread = 0; thread < myMaxThreads; thread++)
		{
			myGraphSecCmdPools.push_back(myDevice.createCommandPool({ { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, myQueues.myGraphicsFamIndex }));
		}
		for(PerThreadCmdBuffers& cmdBuffers : mySecCmdBuffers)
		{
			for (uint32_t thread = 0; thread < myMaxThreads; thread++)
			{
				cmdBuffers.push_back(myDevice.allocateCommandBuffers({ myGraphSecCmdPools[thread], vk::CommandBufferLevel::eSecondary, (uint32_t)ourShadersToLoad.size() }));
			}
		}
	}

	// we will need semaphores for properly managing the async drawing
	myImgAvailable = myDevice.createSemaphore({});
	myRenderFinished = myDevice.createSemaphore({});

	{
		// our 3 fences to synchronize command submission
		for(vk::Fence& fence : myCmdFences)
		{
			fence = myDevice.createFence({ vk::FenceCreateFlagBits::eSignaled });
		}
	}
}

void GraphicsVK::CreateDepthTexture()
{
	// create the image of depth format
	CreateImage(mySwapInfo.mySwapExtent.width, mySwapInfo.mySwapExtent.height, myDepthFormat,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, 
		vk::MemoryPropertyFlagBits::eDeviceLocal, myDepthImg, myDepthImgMem);
	myDepthImgView = CreateImageView(myDepthImg, myDepthFormat, vk::ImageAspectFlagBits::eDepth);
	
	// in order to use it we have to transition it to optimal attachment layout
	TransitionLayout(myDepthImg, myDepthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

vk::Format GraphicsVK::FindSupportedFormat(vk::ImageTiling aTiling, vk::FormatFeatureFlags aFeatures) const
{
	for (vk::Format format : myDepthCands)
	{
		vk::FormatProperties props = myPhysDevice.getFormatProperties(format);
		if (aTiling == vk::ImageTiling::eLinear
			&& (props.linearTilingFeatures & aFeatures) == aFeatures)
		{
			return format;
		}
		else if (aTiling == vk::ImageTiling::eOptimal
			&& (props.optimalTilingFeatures & aFeatures) == aFeatures)
		{
			return format;
		}
	}
	// TODO: get rid of exceptions! Then add a disable exceptions flag for compiler
	throw runtime_error("Failed to find supported format");
}

void GraphicsVK::CreateDescriptorPool()
{
	// at the moment only support uniform buffers, but should add img views as well
	const uint32_t trippleBufferedGOs = Game::maxObjects * 3;
	array<vk::DescriptorPoolSize, 2> poolSizes{
		vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, trippleBufferedGOs }, // to enable tripple buffering
		vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, (uint32_t)ourTexturesToLoad.size() }
	};
	vk::DescriptorPoolCreateInfo poolInfo{
		{}, trippleBufferedGOs + (uint32_t)ourTexturesToLoad.size(), (uint32_t)poolSizes.size(), poolSizes.data()
	};
	myDescriptorPool = myDevice.createDescriptorPool(poolInfo);

	// first, the matrices ubo layout
	vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	};
	myUboLayout = myDevice.createDescriptorSetLayout({ {}, (uint32_t)bindings.size(), bindings.data() });

	// then the samplers
	bindings = {
		vk::DescriptorSetLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
	};
	mySamplerLayout = myDevice.createDescriptorSetLayout({ {}, (uint32_t)bindings.size(), bindings.data() });
}

void GraphicsVK::CreateDescriptorSet()
{
	// every object will have it's own descriptor set, supporting tripple buffering
	const uint32_t trippleBufferedGOs = Game::maxObjects * 3;
	array<vk::DescriptorSetLayout, trippleBufferedGOs> uboLayouts;
	uboLayouts.fill(myUboLayout);

	{
		vk::DescriptorSetAllocateInfo info{
			myDescriptorPool, (uint32_t)uboLayouts.size(), uboLayouts.data()
		};
		DescriptorSets tempUboSets = myDevice.allocateDescriptorSets(info);
		size_t buffer = 0;
		for (vector<vk::DescriptorSet>& uboSet : myUboSets)
		{
			uboSet.insert(uboSet.begin(), tempUboSets.begin() + buffer * Game::maxObjects, tempUboSets.begin() + (buffer + 1) * Game::maxObjects);
			buffer++;
		}
	}

	vector<vk::WriteDescriptorSet> writeTargets;
	{
		// now we have to update the descriptor's values location
		vector<vk::DescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(trippleBufferedGOs);
		writeTargets.reserve(trippleBufferedGOs);
		size_t buffer = 0;
		for (vector<vk::DescriptorSet>& uboSet : myUboSets)
		{
			for (size_t i = 0; i < Game::maxObjects; i++)
			{
				// make descriptor point at buffer
				bufferInfos.push_back({
					myUbo, GetAlignedOffset(Game::maxObjects * buffer + i, sizeof(MatUBO)), sizeof(MatUBO)
					});
				// update descriptor with buffer binding
				writeTargets.push_back({
					uboSet[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer,
					nullptr, &bufferInfos[Game::maxObjects * buffer + i], nullptr
					});
			}
			buffer++;
		}
		myDevice.updateDescriptorSets(writeTargets, nullptr);
	}

	{
		// now the samplers
		vector<vk::DescriptorSetLayout> samplerLayouts(ourTexturesToLoad.size(), mySamplerLayout);
		vk::DescriptorSetAllocateInfo info = {
			myDescriptorPool, (uint32_t)samplerLayouts.size(), samplerLayouts.data()
		};
		mySamplerSets = myDevice.allocateDescriptorSets(info);
	}

	writeTargets.clear();
	{
		// samplers allocated, need to update them
		vector<vk::DescriptorImageInfo> imageInfos;
		imageInfos.reserve(ourTexturesToLoad.size());
		for (uint32_t i = 0; i < ourTexturesToLoad.size(); i++)
		{
			imageInfos.push_back({
				mySampler, myTextureViews[i], vk::ImageLayout::eShaderReadOnlyOptimal
				});
			writeTargets.push_back({
				mySamplerSets[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler,
				&imageInfos[i], nullptr, nullptr
				});
		}
		myDevice.updateDescriptorSets(writeTargets, nullptr);
	}
}

void GraphicsVK::CreateUBO()
{
	size_t uboSize = GetAlignedOffset(Game::maxObjects * 3, sizeof(MatUBO));
	std::println("[Info] Ubo size: {}(for {}*3, min alignment: {})", uboSize, Game::maxObjects, myLimits.minUniformBufferOffsetAlignment);
	CreateBuffer(uboSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, myUbo, myUboMem);

	void* mappedMemStart = myDevice.mapMemory(myUboMem, 0, uboSize, {});
	size_t buffer = 0;
	for (void*& memAddr : myMappedUboMem)
	{
		memAddr = static_cast<char*>(mappedMemStart) + GetAlignedOffset(Game::maxObjects * buffer, sizeof(MatUBO));
		buffer++;
	}
}

void GraphicsVK::DestroyUBO()
{
	if (*myMappedUboMem.begin())
	{
		myDevice.unmapMemory(myUboMem);
		*myMappedUboMem.begin() = nullptr;

		myDevice.destroyBuffer(myUbo);
		myDevice.freeMemory(myUboMem);
	}
}

size_t GraphicsVK::GetAlignedOffset(size_t anInd, size_t aStep) const
{
	const float alignFraction = ceil(aStep * 1.f / myLimits.minUniformBufferOffsetAlignment);
	return static_cast<size_t>(anInd * alignFraction * myLimits.minUniformBufferOffsetAlignment);
}

void GraphicsVK::CreateImage(uint32_t aWidth, uint32_t aHeight, vk::Format aFormat, vk::ImageTiling aTiling, vk::ImageUsageFlags aUsage, vk::MemoryPropertyFlags aMemProps, vk::Image& anImg, vk::DeviceMemory& aMem) const
{
	vk::ImageCreateInfo imgInfo {
		{}, vk::ImageType::e2D, aFormat, vk::Extent3D { aWidth, aHeight, 1},
		1, 1, vk::SampleCountFlagBits::e1, aTiling, aUsage,
		vk::SharingMode::eExclusive, 1, nullptr, 
		vk::ImageLayout::ePreinitialized
	};
	anImg = myDevice.createImage(imgInfo);
	vk::MemoryRequirements reqs = myDevice.getImageMemoryRequirements(anImg);
	vk::MemoryAllocateInfo memInfo {
		reqs.size, FindMemoryType(reqs.memoryTypeBits, aMemProps)
	};
	aMem = myDevice.allocateMemory(memInfo);
	myDevice.bindImageMemory(anImg, aMem, 0);
}

void GraphicsVK::TransitionLayout(vk::Image anImg, vk::Format aFormat, vk::ImageLayout anOldLayout, vk::ImageLayout aNewLayout) const
{
	vk::CommandBuffer cmdBuffer = CreateOneTimeCmdBuffer(vk::CommandBufferLevel::ePrimary);
	
	vk::ImageMemoryBarrier barrier;
	// transfer from - to
	barrier.oldLayout = anOldLayout;
	barrier.newLayout = aNewLayout;
	// we don't transfer ownership of image
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// the image and it's subresource to perform the transition on
	barrier.image = anImg;

	// accounting that depth images can be transitioned as well
	if (aNewLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		// it can be a depth-stencil
		if (HasStencilComponent(aFormat))
		{
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	// now, depending on layout transitions we need to change access masks
	if (anOldLayout == vk::ImageLayout::ePreinitialized 
		&& aNewLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
	}
	else if (anOldLayout == vk::ImageLayout::ePreinitialized 
		&& aNewLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	}
	else if (anOldLayout == vk::ImageLayout::eTransferDstOptimal 
		&& aNewLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	}
	else if (anOldLayout == vk::ImageLayout::eUndefined 
		&& aNewLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits();
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	}
	else
	{
		ASSERT_STR(false, "Transition image layout unsupported : %d -> %d\n", anOldLayout, aNewLayout);
	}

	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
		{},
		0, nullptr,
		0, nullptr,
		1, &barrier);

	EndOneTimeCmdBuffer(cmdBuffer);
}

// images must already be in Transfer Src/Dst Optimal layout
void GraphicsVK::CopyImage(vk::Buffer aSrcBuffer, vk::Image aDstImage, uint32_t aWidth, uint32_t aHeight) const
{
	vk::CommandBuffer cmdBuffer = CreateOneTimeCmdBuffer(vk::CommandBufferLevel::ePrimary);
	
	vk::ImageSubresourceLayers layers {
		vk::ImageAspectFlagBits::eColor, 0, 0, 1
	};

	vk::BufferImageCopy region {
		0, 0, 0, // no offset, tightly packed rows and columns
		layers, {}, { aWidth, aHeight, 1}
	};
	cmdBuffer.copyBufferToImage(aSrcBuffer, aDstImage, vk::ImageLayout::eTransferDstOptimal,
		1, &region);

	EndOneTimeCmdBuffer(cmdBuffer);
}

vk::ImageView GraphicsVK::CreateImageView(vk::Image anImg, vk::Format aFormat, vk::ImageAspectFlags anAspect) const
{
	vk::ImageSubresourceRange range{
		anAspect, 0, 1, 0, 1
	};

	vk::ImageViewCreateInfo info{
		{}, anImg, vk::ImageViewType::e2D, aFormat, {}, range
	};
	return myDevice.createImageView(info);
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
	mySampler = myDevice.createSampler(info);
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
		vk::VertexInputAttributeDescription { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, myPos) }, 
		// vec2 uv
		vk::VertexInputAttributeDescription { 1, 0, vk::Format::eR32G32Sfloat,    offsetof(Vertex, myUv) }, 
		// vec3 normal
		vk::VertexInputAttributeDescription { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, myNormal) }, 
	};
	return descs;
}

void GraphicsVK::CreateBuffer(vk::DeviceSize aSize, vk::BufferUsageFlags aUsage, vk::MemoryPropertyFlags aMemProps, vk::Buffer& aBuff, vk::DeviceMemory& aMem) const
{
	// create vbo
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = aSize;
	bufferInfo.usage = aUsage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	uint32_t famIndices[2] = { myQueues.myGraphicsFamIndex, myQueues.myTransferFamIndex };
	bufferInfo.queueFamilyIndexCount = 2;
	bufferInfo.pQueueFamilyIndices = famIndices;
	aBuff = myDevice.createBuffer(bufferInfo);

	// buffer doesn't have any memmory assigned to it yet
	vk::MemoryRequirements reqs = myDevice.getBufferMemoryRequirements(aBuff);
	vk::MemoryAllocateInfo allocInfo {
		reqs.size,
		FindMemoryType(reqs.memoryTypeBits,	aMemProps)
	};
	aMem = myDevice.allocateMemory(allocInfo);
	// now that we have the memory for it created, we have to bind it to buffer
	myDevice.bindBufferMemory(aBuff, aMem, 0);
}

void GraphicsVK::CopyBuffer(vk::Buffer aFrom, vk::Buffer aTo, vk::DeviceSize aSize) const
{
	// to copy it over a command buffer is needed
	vk::CommandBuffer cmdBuffer = CreateOneTimeCmdBuffer(vk::CommandBufferLevel::ePrimary);
	vk::BufferCopy region {
		0, 0, aSize
	};
	cmdBuffer.copyBuffer(aFrom, aTo, 1, &region);
	EndOneTimeCmdBuffer(cmdBuffer);
}

uint32_t GraphicsVK::FindMemoryType(uint32_t aTypeFilter, vk::MemoryPropertyFlags aProps) const
{
	vk::PhysicalDeviceMemoryProperties devProps = myPhysDevice.getMemoryProperties();
	for (uint32_t i = 0; i < devProps.memoryTypeCount; i++)
	{
		if (aTypeFilter & (1 << i)
			&& (devProps.memoryTypes[i].propertyFlags & aProps) == aProps)
		{
			return i;
		}
	}
	return -1;
}

vk::CommandBuffer GraphicsVK::CreateOneTimeCmdBuffer(vk::CommandBufferLevel aLevel) const
{
	vk::CommandBufferAllocateInfo info{
		myGraphCmdPool, aLevel, 1
	};
	vk::CommandBuffer cmdBuffer = myDevice.allocateCommandBuffers(info)[0];

	vk::CommandBufferBeginInfo beginInfo{
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr
	};
	cmdBuffer.begin(beginInfo);
	return cmdBuffer;
}

void GraphicsVK::EndOneTimeCmdBuffer(vk::CommandBuffer aCmdBuff) const
{
	aCmdBuff.end();
	
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &aCmdBuff;
	myQueues.myGraphicsQueue.submit(1, &submitInfo, VK_NULL_HANDLE);
	myQueues.myGraphicsQueue.waitIdle();
	
	myDevice.freeCommandBuffers(myGraphCmdPool, 1, &aCmdBuff);
}

bool GraphicsVK::LayersAvailable(const vector<const char*> &aValidationLayersList) const
{
	//what we have
	uint32_t availableLayersCount;
	vk::enumerateInstanceLayerProperties(&availableLayersCount, nullptr);

	vector<vk::LayerProperties> availableLayers(availableLayersCount);
	vk::enumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());

	// validation layers lookup
	bool foundAll = true;
	for (const char* layerRequested : aValidationLayersList)
	{
		bool found = false;
		for (vk::LayerProperties layerAvailable : availableLayers)
		{
			if (strcmp(layerAvailable.layerName, layerRequested) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			std::println("[Warning] Didn't find validation layer: {}", layerRequested);
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
		std::println("[Error] VL: {}", msg);
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::println("[Warning] VL: {}", msg);
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		std::println("[Performance] VL: {}", msg);
	return VK_FALSE;
}

#endif // USE_VULKAN