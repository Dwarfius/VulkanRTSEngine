#include "Precomp.h"
#include "GraphicsGL.h"

#include "Terrain.h"
#include "VisualObject.h"
#include "Graphics/Adapters/UniformAdapter.h"
#include "ShaderGL.h"
#include "PipelineGL.h"
#include "ModelGL.h"
#include "TextureGL.h"
#include "UniformBufferGL.h"
#include "RenderPassJobGL.h"

#include <sstream>

#include <Core/Debug/DebugDrawer.h>

#include <Graphics/Camera.h>
#include <Graphics/AssetTracker.h>
#include <Graphics/Resources/Shader.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/GPUResource.h>

#ifdef _DEBUG
#define DEBUG_GL_CALLS
#endif

#ifdef DEBUG_GL_CALLS
void APIENTRY glDebugOutput(GLenum, GLenum, GLuint, GLenum,
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
	ourActiveGraphics = this;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
#ifdef DEBUG_GL_CALLS
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	myWindow = glfwCreateWindow(ourWidth, ourHeight, "VEngine - GL", nullptr, nullptr);
	glfwSetInputMode(myWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowSizeCallback(myWindow, GraphicsGL::OnWindowResized);
	
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

	{
		Handle<Pipeline> debugPipeline = myAssetTracker.GetOrCreate<Pipeline>("debug.ppl");
		myDebugPipeline = GetOrCreate(debugPipeline).Get<PipelineGL>();
	}

	CreateLineCache();

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
	Graphics::Display();

	myRenderPassJobs.Advance();

	// TEST
	glBeginQuery(GL_PRIMITIVES_GENERATED, myGPUQuery);
	// ======
	const RenderPassJobMap& jobs = myRenderPassJobs.GetRead();
	for (const auto& pair : jobs)
	{
		pair.second->Execute();
	}

	// TEST
	glEndQuery(GL_PRIMITIVES_GENERATED);
	uint32_t triNum;
	glGetQueryObjectuiv(myGPUQuery, GL_QUERY_RESULT, &triNum);
	//std::printf("Triangles: %u\n", triNum);
	// ======

	// lastly going to process the debug lines
	if(myDebugPipeline->GetState() == GPUResource::State::Valid
		&& myLineCache.myUploadDesc.myPrimitiveCount > 0)
	{
		// upload the entire chain
		Model* debugModel = myLineCache.myBuffer->GetResource().Get<Model>();
		debugModel->Update(myLineCache.myUploadDesc);
		Graphics::TriggerUpload(myLineCache.myBuffer.Get());
		
		// clean up the upload descriptors
		for (LineCache::UploadDesc* currDesc = &myLineCache.myUploadDesc;
			currDesc != nullptr;
			currDesc = currDesc->myNextDesc)
		{
			currDesc->myVertCount = 0;
		}

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

	glfwSwapBuffers(myWindow);
}

void GraphicsGL::CleanUp()
{
	myDebugPipeline = Handle<PipelineGL>();
	myLineCache.myBuffer = Handle<ModelGL>();

	constexpr uint32_t kMaxExtraDescriptors = 32;
	LineCache::UploadDesc* descriptors[kMaxExtraDescriptors];
	uint32_t descInd = 0;
	for (LineCache::UploadDesc* currDesc = myLineCache.myUploadDesc.myNextDesc;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		descriptors[descInd++] = currDesc;
	}
	while (descInd > 0)
	{
		delete descriptors[--descInd];
	}

	UnloadAll();

	glfwDestroyWindow(myWindow);
	ourActiveGraphics = nullptr;
}

GPUResource* GraphicsGL::Create(Resource::Type aType) const
{
	switch (aType)
	{
	case Resource::Type::Model:		return new ModelGL();
	case Resource::Type::Pipeline:	return new PipelineGL();
	case Resource::Type::Shader:	return new ShaderGL();
	case Resource::Type::Texture:	return new TextureGL();
	default:	ASSERT_STR(false, "Unrecognized GPU resource requested!");
	}
	return nullptr;
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
	currDesc->myPrimitiveCount = aDebugDrawer.GetCurrentVertexCount();
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

	((GraphicsGL*)ourActiveGraphics)->OnResize(aWidth, aHeight);
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
}

void GraphicsGL::CreateLineCache()
{
	myLineCache.myBuffer = new ModelGL(PrimitiveType::Lines, GPUResource::UsageType::Dynamic, PosColorVertex::Type, false);
	Handle<Model> cpuBuffer = new Model(PrimitiveType::Lines, PosColorVertex::Type);
	myLineCache.myBuffer->Create(*this, cpuBuffer, true);
}

#ifdef DEBUG_GL_CALLS
void APIENTRY glDebugOutput(GLenum aSource,
	GLenum aType,
	GLuint aId,
	GLenum aSeverity,
	GLsizei aLength,
	const GLchar* aMessage,
	const void* aUserParam)
{
	char* source;
	switch (aSource)
	{
	case GL_DEBUG_SOURCE_API:             source = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source = "Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: source = "Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     source = "Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     source = "Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           source = "Other"; break;
	}

	char* type;
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
	char* severity;
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