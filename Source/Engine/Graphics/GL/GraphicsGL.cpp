#include "Precomp.h"
#include "GraphicsGL.h"

#include "Terrain.h"
#include "VisualObject.h"
#include "ShaderGL.h"
#include "PipelineGL.h"
#include "ModelGL.h"
#include "TextureGL.h"
#include "GPUBufferGL.h"
#include "RenderPassJobGL.h"
#include "Graphics/NamedFrameBuffers.h"

#include <Core/Debug/DebugDrawer.h>
#include <Core/Resources/AssetTracker.h>
#include <Core/Profiler.h>
#include <Core/Utils.h>

#include <Graphics/Camera.h>
#include <Graphics/Resources/Shader.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/GPUResource.h>

#ifdef _DEBUG
#define DEBUG_GL_CALLS
#endif

#ifdef DEBUG_GL_CALLS
void GLAPIENTRY glDebugOutput(GLenum, GLenum, GLuint, GLenum,
	GLsizei, const GLchar*, const void*);
#endif

template<>
struct std::formatter<const GLubyte*>
{
	template<class Context>
	constexpr Context::iterator parse(Context& aCtx)
	{
		std::formatter<const char*> fmt;
		return fmt.parse(aCtx);
	}

	template<class Context>
	Context::iterator format(const GLubyte* aString, Context& aCtx) const
	{
		std::formatter<const char*> fmt;
		const char* str = reinterpret_cast<const char*>(aString);
		return fmt.format(str, aCtx);
	}
};

GraphicsGL::GraphicsGL(AssetTracker& anAssetTracker)
	: Graphics(anAssetTracker)
{
	for (uint8_t i = 0; i < GraphicsConfig::kMaxFramesScheduled - 1; i++)
	{
		myGPUBufferCleanUpQueue.AdvanceWrite();
	}
}

void GraphicsGL::Init()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
#ifdef DEBUG_GL_CALLS
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	int width = static_cast<int>(GetWidth());
	int height = static_cast<int>(GetHeight());
	myWindow = glfwCreateWindow(width, height, "VEngine - GL", nullptr, nullptr);
	glfwSetWindowSizeCallback(myWindow, GraphicsGL::OnWindowResized);
	glfwSetWindowUserPointer(myWindow, this);
	
	glfwMakeContextCurrent(myWindow);
	glfwSwapInterval(0); // turning off vsync
	glewExperimental = true;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::println("[Error] GLEW failed to init, error: {}", glewGetErrorString(err));
		return;
	}
	glGetError(); // skip the glew error
	std::println("[Info] OpenGL context found, version: {}", glGetString(GL_VERSION));
	std::println("[Info] GLEW initialized, version: {}", glewGetString(GLEW_VERSION));

#ifdef DEBUG_GL_CALLS
	// check if the GL context has been provided
	int flags; 
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		// yeah, we got it, so hook in our handler
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		// Enable every output
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 
			0, nullptr, GL_TRUE);
		// except for notification level
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 
			0, nullptr, GL_FALSE);
	}
#endif

	// Configuring all unpacking from client-side to be byte aligned
	// This prevents problems from some textures not being word aligned
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Enabled by default - and some workflows need it off
	// and I don't see the difference, so gonna skip this stge
	glDisable(GL_DITHER);

	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &myUBOOffsetAlignment);

	Graphics::Init();
}

void GraphicsGL::Display()
{
	Profiler::ScopedMark profile("GraphicsGL::Display");
	Graphics::Display();

	{
		myGPUBufferCleanUpQueue.AdvanceRead();
		tbb::concurrent_queue<GPUBufferGL*>& queue = myGPUBufferCleanUpQueue.GetRead();
		GPUBufferGL* buffer;
		while (queue.try_pop(buffer))
		{
			ASSERT_STR(myGPUBuffers.Contains(buffer), "Unrecognized UBO - Where did it come from?");
			TriggerUnload(buffer);
			myGPUBuffers.Free(*buffer);
		}
	}

	{
		myGPUBuffers.ForEach([](GPUBufferGL& aUBO) {
			aUBO.AdvanceReadBuffer();
		});
	}

	{
		Profiler::ScopedMark profile("GraphicsGL::WaitForUBOSync");
		// Before starting to execute accumulated jobs
		// we need to wait for completion of last frame
		// when mapped region was used previously (3 frames ago)
		// otherwise we risk posioning the UBOs
		// uploads of which might've been deferred by the driver
		GLsync fence = static_cast<GLsync>(myFrameFences[myReadFenceInd]);
		if (fence) [[likely]]
		{
			GLenum status;
			do
			{
				status = glClientWaitSync(fence, 0, 0);
			} while (status != GL_ALREADY_SIGNALED);
		}
		myReadFenceInd++;
		myReadFenceInd %= GraphicsConfig::kMaxFramesScheduled + 1;
	}

	myRenderPassJobs.AdvanceRead();
	{
		Profiler::ScopedMark profile("GraphicsGL::ExecuteJobs");
		RenderPassJobs& jobs = myRenderPassJobs.GetRead();
		const uint32_t maxJobInd = jobs.myJobCounter;
		for (uint32_t i=0; i<maxJobInd; i++)
		{
			jobs.myJobs[i].Execute(*this);
		}
	}

	{
		glDeleteSync(static_cast<GLsync>(myFrameFences[myWriteFenceInd]));
		myFrameFences[myWriteFenceInd] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		myWriteFenceInd++;
		myWriteFenceInd %= GraphicsConfig::kMaxFramesScheduled + 1;
	}

	{
		Profiler::ScopedMark swapProfile("GraphicsGL::SwapBuffers");
		glfwSwapBuffers(myWindow);
	}
}

void GraphicsGL::Gather()
{
	myGPUBufferCleanUpQueue.AdvanceWrite();
	
	Graphics::Gather();

	myRenderPassJobs.AdvanceWrite();
	myRenderPassJobs.GetWrite().myJobCounter = 0;
}

void GraphicsGL::CleanUp()
{
	Graphics::CleanUp();

	for(tbb::concurrent_queue<GPUBufferGL*>& queue : myGPUBufferCleanUpQueue)
	{
		GPUBufferGL* buffer;
		while (queue.try_pop(buffer))
		{
			ASSERT_STR(myGPUBuffers.Contains(buffer), "Unrecognized UBO - Where did it come from?");
			TriggerUnload(buffer);
			myGPUBuffers.Free(*buffer);
		}
	}

	ProcessAllUnloadQueues();
	ASSERT_STR(AreResourcesEmpty(), "Leak! Everything should've been cleaned up!");

	glfwDestroyWindow(myWindow);
}

GPUResource* GraphicsGL::Create(Model*, GPUResource::UsageType aUsage)
{
	std::lock_guard lock(myModelsMutex);
	return &myModels.Allocate(aUsage);
}

GPUResource* GraphicsGL::Create(Pipeline*, GPUResource::UsageType)
{
	std::lock_guard lock(myPipelinesMutex);
	return &myPipelines.Allocate();
}

GPUResource* GraphicsGL::Create(Texture*, GPUResource::UsageType)
{
	std::lock_guard lock(myTexturesMutex);
	return &myTextures.Allocate();
}

GPUResource* GraphicsGL::Create(Shader*, GPUResource::UsageType)
{
	return new ShaderGL();
}

GPUBuffer* GraphicsGL::CreateGPUBufferImpl(size_t aSize, uint8_t aFrameCount, bool aIsUBO)
{
	const size_t alignedSize = aIsUBO ? Utils::Align(aSize, myUBOOffsetAlignment) : aSize;

	// TODO: add recycling to avoid recreating GPU-side resources
	std::lock_guard lock(myGPUBuffersMutex);
	return &myGPUBuffers.Allocate(alignedSize, aFrameCount);
}

void GraphicsGL::OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight)
{
	if (aWidth == 0 && aHeight == 0)
	{
		return;
	}

	GraphicsGL* graphics = static_cast<GraphicsGL*>(glfwGetWindowUserPointer(aWindow));
	graphics->OnResize(aWidth, aHeight);
}

void GraphicsGL::AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBuffer)
{
	Graphics::AddNamedFrameBuffer(aName, aBuffer);

	myFrameBuffers.emplace(aName, FrameBufferGL(*this, aBuffer));
}

void GraphicsGL::ResizeNamedFrameBuffer(std::string_view aName, glm::ivec2 aSize)
{
	Graphics::ResizeNamedFrameBuffer(aName, aSize);

	auto iter = myFrameBuffers.find(aName);
	ASSERT_STR(iter != myFrameBuffers.end(),
		"FrameBuffer {} not registered!", aName);
	iter->second.SetIsFullScreen(aSize == FrameBuffer::kFullScreen);
	iter->second.Resize(aSize.x, aSize.y);
}

FrameBufferGL& GraphicsGL::GetFrameBufferGL(std::string_view aName)
{
	auto iter = myFrameBuffers.find(aName);
	ASSERT_STR(iter != myFrameBuffers.end(), 
		"FrameBuffer with name {} doesn't exist!", aName);
	return iter->second;
}

RenderPassJob& GraphicsGL::CreateRenderPassJob(const RenderContext& renderContext)
{
	RenderPassJobs& jobs = myRenderPassJobs.GetWrite();

	ASSERT_STR(jobs.myJobCounter != kMaxRenderPassJobs,
		"Exhausted capacity render pass jobs({}), please increase "
		"GraphicsGL::kMaxRenderPassJobs!", kMaxRenderPassJobs);

	RenderPassJobGL& job = jobs.myJobs[jobs.myJobCounter++];
	job.Initialize(renderContext);
	return job;
}

void GraphicsGL::CleanUpGPUBuffer(GPUBuffer* aUBO)
{
	ASSERT_STR(aUBO->GetState() == GPUResource::State::PendingUnload,
		"UBO must be marked as end-of-life at this point!");
	UnregisterResource(aUBO); // TODO: can we avoid unregister here? Those are UBOs!
	myGPUBufferCleanUpQueue.GetWrite().push(static_cast<GPUBufferGL*>(aUBO));
}

void GraphicsGL::OnResize(int aWidth, int aHeight)
{
	Graphics::OnResize(aWidth, aHeight);

	const uint32_t width = static_cast<uint32_t>(aWidth);
	const uint32_t height = static_cast<uint32_t>(aHeight);
	for (auto& [key, frameBuffer] : myFrameBuffers)
	{
		if (frameBuffer.IsFullScreen())
		{
			frameBuffer.Resize(width, height);
		}
	}
}

void GraphicsGL::DeleteResource(GPUResource* aResource)
{
	// TODO: replace the dynamic_casts with compile time paths.
	// Implemented this way for now to test the performance improvement
	// of pooling resources
	if (GPUPipeline* pipeline = dynamic_cast<GPUPipeline*>(aResource))
	{
		std::lock_guard lock(myPipelinesMutex);
		myPipelines.Free(*static_cast<PipelineGL*>(pipeline));
	}
	else if (GPUModel* model = dynamic_cast<GPUModel*>(aResource))
	{
		std::lock_guard lock(myModelsMutex);
		myModels.Free(*static_cast<ModelGL*>(model));
	}
	else if (GPUTexture* texture = dynamic_cast<GPUTexture*>(aResource))
	{
		std::lock_guard lock(myTexturesMutex);
		myTextures.Free(*static_cast<TextureGL*>(texture));
	}
	else
	{
		Graphics::DeleteResource(aResource);
	}
}

#ifdef DEBUG_GL_CALLS
void GLAPIENTRY glDebugOutput(GLenum aSource,
	GLenum aType,
	GLuint aId,
	GLenum aSeverity,
	GLsizei aLength,
	const GLchar* aMessage,
	const void* aUserParam)
{
	switch (aId)
	{
	case 131204: 
		// "Unbound sampler or badly defined texture at sampler"
		// Engine marks slots for use in a render pass 
		// even if they might not end up all being used 
	case 131220: 
		// "A fragment program/shader is required to correctly 
		// render to an integer framebuffer."
		// Can happen during a call to Clear with a uint framebuffer
		return; 
	default: break;
	}

	const char* source;
	switch (aSource)
	{
	case GL_DEBUG_SOURCE_API:             source = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source = "Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: source = "Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     source = "Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     source = "Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           source = "Other"; break;
	}

	const char* type;
	switch (aType)
	{
	case GL_DEBUG_TYPE_ERROR:               type = "Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type = "Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type = "Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         type = "Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         type = "Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              type = "Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          type = "Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           type = "Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               type = "Other"; break;
	}

	bool shouldAssert = false;
	const char* severity;
	switch (aSeverity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         severity = "high"; shouldAssert = true; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       severity = "medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          severity = "low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: severity = "notification"; break;
	}
	
	if (shouldAssert)
	{
		ASSERT_STR(false, "Severe GL Error({}): {}\nSource: {}\nType: {}",
			aId, aMessage, source, type);
	}
	else
	{
		std::println("=================");
		std::println("GL Debug({}): {}", aId, aMessage);
		std::println("Source: {}", source);
		std::println("Type: {}", type);
		std::println("Severity: {}", severity);
	}
}
#endif