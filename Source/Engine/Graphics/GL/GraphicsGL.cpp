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

#include <Core/Debug/DebugDrawer.h>
#include <Core/Resources/AssetTracker.h>
#include <Core/Profiler.h>

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
	, myDebugPipeline(nullptr)
{
	memset(&myLineCache, 0, sizeof(myLineCache));
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

	myWindow = glfwCreateWindow(ourWidth, ourHeight, "VEngine - GL", nullptr, nullptr);
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

	CreateLineCache();
	CreateFrameQuad();

	// TEST
	glGenQueries(1, &myGPUQuery);
	// ===========
}

void GraphicsGL::BeginGather()
{
	Graphics::BeginGather();
}

void GraphicsGL::Display()
{
	Profiler::ScopedMark profile("GraphicsGL::Display");
	Graphics::Display();

	myRenderPassJobs.Advance();

	{
		Profiler::ScopedMark profile("GraphicsGL::ExecuteJobs");
		const RenderPassJobMap& jobs = myRenderPassJobs.GetRead();
		for (const auto& pair : jobs)
		{
			pair.second->Execute(*this);
		}
	}

	// lastly going to process the debug lines
	{
		Profiler::ScopedMark debugProfile("GraphicsGL::DebugRender");

		const bool isPipelineReady = myDebugPipeline->GetState() == GPUResource::State::Valid;
		const bool hasDebugData = myLineCache.myUploadDesc.myVertCount > 0;
		if (hasDebugData)
		{
			if (isPipelineReady)
			{
				// upload the entire chain
				Model* debugModel = myLineCache.myBuffer->GetResource().Get<Model>();
				debugModel->Update(myLineCache.myUploadDesc);
				Graphics::TriggerUpload(myLineCache.myBuffer.Get());
			}

			// clean up the upload descriptors
			for (LineCache::UploadDesc* currDesc = &myLineCache.myUploadDesc;
				currDesc != nullptr;
				currDesc = currDesc->myNextDesc)
			{
				currDesc->myVertCount = 0;
			}
		}

		// TODO: replace with a DebugRenderPass
		if (isPipelineReady && hasDebugData)
		{
			glDisable(GL_SCISSOR_TEST);

			// shader first
			myDebugPipeline->Bind();

			const GLfloat* vp = static_cast<const GLfloat*>(glm::value_ptr(myLineCache.myVp));
			glUniformMatrix4fv(0, 1, false, vp);

			// then VAO
			myLineCache.myBuffer->Bind();

			// now just draw em out
			GLsizei count = static_cast<GLsizei>(myLineCache.myBuffer->GetVertexCount());
			glDrawArrays(myLineCache.myBuffer->GetDrawMode(), 0, count);
		}
	}

	if(myCompositePipeline->GetState() == GPUResource::State::Valid)
	{
		// TODO: make it a renderpass
		// performing final compose
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		myCompositePipeline->Bind();
		myFrameQuad->Bind();

		FrameBufferGL& buffer = GetFrameBufferGL("Default");
		uint32_t texture = buffer.GetColorTexture(0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		GLsizei count = static_cast<GLsizei>(myFrameQuad->GetVertexCount());
		glDrawArrays(myFrameQuad->GetDrawMode(), 0, count);
	}

	{
		Profiler::ScopedMark swapProfile("GraphicsGL::SwapBuffers");
		glfwSwapBuffers(myWindow);
	}
}

void GraphicsGL::CleanUp()
{
	Graphics::CleanUp();

	myDebugPipeline = Handle<PipelineGL>();
	myLineCache.myBuffer = Handle<ModelGL>();

	myFrameQuad = Handle<ModelGL>();
	myCompositePipeline = Handle<PipelineGL>();

	constexpr uint32_t kMaxExtraDescriptors = 32;
	LineCache::UploadDesc* descriptors[kMaxExtraDescriptors];
	uint32_t descInd = 0;
	for (LineCache::UploadDesc* currDesc = myLineCache.myUploadDesc.myNextDesc;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		ASSERT_STR(descInd < kMaxExtraDescriptors, "Descriptor buffer not big enough!");
		descriptors[descInd++] = currDesc;
	}
	while (descInd > 0)
	{
		delete descriptors[--descInd];
	}

	// clean up all render pass jobs
	for (RenderPassJobMap& map : myRenderPassJobs)
	{
		for (auto [key, job] : map)
		{
			delete job;
		}
	}

	ProcessUnloadQueue();
	ASSERT_STR(AreResourcesEmpty(), "Leak! Everything should've been cleaned up!");

	glfwDestroyWindow(myWindow);
}

GPUResource* GraphicsGL::Create(Model*) const
{
	return new ModelGL();
}

GPUResource* GraphicsGL::Create(Pipeline*) const
{
	return new PipelineGL();
}

GPUResource* GraphicsGL::Create(Shader*) const
{
	return new ShaderGL();
}

GPUResource* GraphicsGL::Create(Texture*) const
{
	return new TextureGL();
}

void GraphicsGL::RenderDebug(const Camera& aCam, const DebugDrawer& aDebugDrawer)
{
	// there's always 1 space allocated for debug drawer, but we might need more
	// if there are more drawers. Look for a slot that's free (doesn't have vertices)
	// or create one if there isn't one.
	LineCache::UploadDesc* currDesc;
	for (currDesc = &myLineCache.myUploadDesc;
		currDesc->myVertCount != 0; // keep iterating until we find an empty one
		currDesc = currDesc->myNextDesc)
	{
		if (currDesc->myNextDesc == nullptr)
		{
			currDesc->myNextDesc = new LineCache::UploadDesc();
			std::memset(currDesc->myNextDesc, 0, sizeof(LineCache::UploadDesc));
		}
	}
	
	// we've found a free one - fill it up
	currDesc->myVertices = aDebugDrawer.GetCurrentVertices();
	currDesc->myVertCount = aDebugDrawer.GetCurrentVertexCount();
	currDesc->myIndices = nullptr;
	currDesc->myIndCount = 0;

	// TODO: this is wasteful - refactor to set it once or cache per debug drawer
	myLineCache.myVp = aCam.Get();
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

void GraphicsGL::AddNamedFrameBuffer(const std::string& aName, const FrameBuffer& aBuffer)
{
	Graphics::AddNamedFrameBuffer(aName, aBuffer);

	myFrameBuffers.emplace(aName, FrameBufferGL(*this, aBuffer));
}

FrameBufferGL& GraphicsGL::GetFrameBufferGL(const std::string& aName)
{
	auto iter = myFrameBuffers.find(aName);
	ASSERT_STR(iter != myFrameBuffers.end(), 
		"FrameBuffer with name %s doesn't exist!", aName.c_str());
	return iter->second;
}

RenderPassJob& GraphicsGL::GetRenderPassJob(uint32_t anId, const RenderContext& renderContext)
{
	RenderPassJob* foundJob;
	RenderPassJobMap& map = myRenderPassJobs.GetWrite();
	auto contextIter = map.find(anId);
	if (contextIter != map.end())
	{
		foundJob = contextIter->second;
	}
	else
	{
		foundJob = new RenderPassJobGL();
		map[anId] = foundJob;
	}
	foundJob->Initialize(renderContext);
	return *foundJob;
}

void GraphicsGL::OnResize(int aWidth, int aHeight)
{
	ourWidth = aWidth;
	ourHeight = aHeight;

	for (auto& [key, frameBuffer] : myFrameBuffers)
	{
		frameBuffer.OnResize(*this);
	}
}

void GraphicsGL::CreateLineCache()
{
	Handle<Pipeline> debugPipeline = myAssetTracker.GetOrCreate<Pipeline>("Engine/debug.ppl");
	myDebugPipeline = GetOrCreate(debugPipeline).Get<PipelineGL>();

	myLineCache.myBuffer = new ModelGL(
		Model::PrimitiveType::Lines, 
		GPUResource::UsageType::Dynamic, 
		PosColorVertex::GetDescriptor(), 
		false
	);
	Model::VertStorage<PosColorVertex>* buffer = new Model::VertStorage<PosColorVertex>(0);
	Handle<Model> cpuBuffer = new Model(Model::PrimitiveType::Lines, buffer, false);
	myLineCache.myBuffer->Create(*this, cpuBuffer, true);
}

void GraphicsGL::CreateFrameQuad()
{
	Handle<Pipeline> pipeline = myAssetTracker.GetOrCreate<Pipeline>("Engine/composite.ppl");
	myCompositePipeline = GetOrCreate(pipeline).Get<PipelineGL>();

	struct PosUVVertex
	{
		glm::vec2 myPos;
		glm::vec2 myUV;

		PosUVVertex() = default;
		constexpr PosUVVertex(glm::vec2 aPos, glm::vec2 aUV)
			: myPos(aPos)
			, myUV(aUV)
		{
		}

		static constexpr VertexDescriptor GetDescriptor()
		{
			using ThisType = PosUVVertex; // for copy-paste convenience
			return {
				sizeof(ThisType),
				2,
				{
					{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myPos) },
					{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myUV) }
				}
			};
		}
	};

	myFrameQuad = new ModelGL(
		Model::PrimitiveType::Triangles,
		GPUResource::UsageType::Static,
		PosUVVertex::GetDescriptor(),
		false
	);
	PosUVVertex vertices[] = {
		{ { -1.f,  1.f }, { 0.f, 1.f } },
		{ { -1.f, -1.f }, { 0.f, 0.f } },
		{ {  1.f, -1.f }, { 1.f, 0.f } },

		{ { -1.f,  1.f }, { 0.f, 1.f } },
		{ {  1.f, -1.f }, { 1.f, 0.f } },
		{ {  1.f,  1.f }, { 1.f, 1.f } }
	};
	Model::VertStorage<PosUVVertex>* buffer = new Model::VertStorage<PosUVVertex>(6, vertices);
	Handle<Model> cpuModel = new Model(Model::PrimitiveType::Triangles, buffer, false);
	myFrameQuad->Create(*this, cpuModel, false);
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