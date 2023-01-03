#include "Precomp.h"
#include "GraphicsGL.h"

#include "Terrain.h"
#include "VisualObject.h"
#include "ShaderGL.h"
#include "PipelineGL.h"
#include "ModelGL.h"
#include "TextureGL.h"
#include "UniformBufferGL.h"
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

GraphicsGL::GraphicsGL(AssetTracker& anAssetTracker)
	: Graphics(anAssetTracker)
{
	for (uint8_t i = 0; i < GraphicsConfig::kMaxFramesScheduled - 1; i++)
	{
		myUBOCleanUpQueues.AdvanceWrite();
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
		printf("[Error] GLEW failed to init, error: %s\n", glewGetErrorString(err));
		return;
	}
	glGetError(); // skip the glew error
	printf("[Info] OpenGL context found, version: %s\n", glGetString(GL_VERSION));
	printf("[Info] GLEW initialized, version: %s\n", glewGetString(GLEW_VERSION));

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
		myUBOCleanUpQueues.AdvanceRead();
		tbb::concurrent_queue<UniformBufferGL*>& queue = myUBOCleanUpQueues.GetRead();
		UniformBufferGL* ubo;
		while (queue.try_pop(ubo))
		{
			ASSERT_STR(myUBOs.Contains(ubo), "Unrecognized UBO - Where did it come from?");
			TriggerUnload(ubo);
			myUBOs.Free(*ubo);
		}
	}

	{
		myUBOs.ForEach([](UniformBufferGL& aUBO) {
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
		for (RenderPassJobGL& job : jobs)
		{
			job.Execute(*this);
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
	myUBOCleanUpQueues.AdvanceWrite();
	
	Graphics::Gather();

	myRenderPassJobs.AdvanceWrite();
	myNextFreeJob = 0;
}

void GraphicsGL::CleanUp()
{
	Graphics::CleanUp();

	for(tbb::concurrent_queue<UniformBufferGL*>& queue : myUBOCleanUpQueues)
	{
		UniformBufferGL* ubo;
		while (queue.try_pop(ubo))
		{
			ASSERT_STR(myUBOs.Contains(ubo), "Unrecognized UBO - Where did it come from?");
			TriggerUnload(ubo);
			myUBOs.Free(*ubo);
		}
	}

	ProcessAllUnloadQueues();
	ASSERT_STR(AreResourcesEmpty(), "Leak! Everything should've been cleaned up!");

	glfwDestroyWindow(myWindow);
}

GPUResource* GraphicsGL::Create(Model*, GPUResource::UsageType aUsage) const
{
	return new ModelGL(aUsage);
}

GPUResource* GraphicsGL::Create(Pipeline*, GPUResource::UsageType) const
{
	return new PipelineGL();
}

GPUResource* GraphicsGL::Create(Texture*, GPUResource::UsageType) const
{
	return new TextureGL();
}

GPUResource* GraphicsGL::Create(Shader*, GPUResource::UsageType) const
{
	return new ShaderGL();
}

UniformBuffer* GraphicsGL::CreateUniformBufferImpl(size_t aSize)
{
	const size_t alignedSize = Utils::Align(aSize, myUBOOffsetAlignment);

	// TODO: add recycling to avoid recreating GPU-side resources
	std::lock_guard lock(myUBOsMutex);
	return &myUBOs.Allocate(alignedSize);
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
		"FrameBuffer %s not registered!", aName.data());
	iter->second.SetIsFullScreen(aSize == FrameBuffer::kFullScreen);
	iter->second.Resize(aSize.x, aSize.y);
}

FrameBufferGL& GraphicsGL::GetFrameBufferGL(std::string_view aName)
{
	auto iter = myFrameBuffers.find(aName);
	ASSERT_STR(iter != myFrameBuffers.end(), 
		"FrameBuffer with name %s doesn't exist!", aName.data());
	return iter->second;
}

RenderPassJob& GraphicsGL::CreateRenderPassJob(const RenderContext& renderContext)
{
	ASSERT_STR(myNextFreeJob != kMaxRenderPassJobs,
		"Exhausted capacity render pass jobs(%u), please increase "
		"GraphicsGL::kMaxRenderPassJobs!", kMaxRenderPassJobs);

	RenderPassJobs& jobs = myRenderPassJobs.GetWrite();
	RenderPassJobGL& job = jobs[myNextFreeJob++];
	job.Initialize(renderContext);
	return job;
}

void GraphicsGL::CleanUpUBO(UniformBuffer* aUBO)
{
	ASSERT_STR(aUBO->GetState() == GPUResource::State::PendingUnload,
		"UBO must be marked as end-of-life at this point!");
	UnregisterResource(aUBO);
	myUBOCleanUpQueues.GetWrite().push(static_cast<UniformBufferGL*>(aUBO));
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
		ASSERT_STR(false, "Severe GL Error(%d): %s\nSource: %s\nType: %s",
			aId, aMessage, source, type);
	}
	else
	{
		printf("=================\nGL Debug(%d): %s\nSource: %s\nType: %s\nSeverity: %s\n",
			aId, aMessage, source, type, severity);
	}
}
#endif