#include "Precomp.h"
#include "GraphicsGL.h"
#include "Camera.h"
#include "Terrain.h"
#include "VisualObject.h"

#include "Graphics/AssetTracker.h"
#include "Graphics/UniformAdapter.h"
#include "ShaderGL.h"
#include "PipelineGL.h"
#include "ModelGL.h"
#include "TextureGL.h"

#define CHECK_GL()															\
{																			\
	const GLenum err = glGetError();										\
	if (err != GL_NO_ERROR)													\
		printf("GL error(%d) on line %d in %s", err, __LINE__, __FILE__);	\
}

GraphicsGL::GraphicsGL(AssetTracker& anAssetTracker)
	: Graphics(anAssetTracker)
	, myCurrentModel(nullptr)
	, myCurrentPipeline(nullptr)
	, myCurrentTexture(nullptr)
	, myDebugVertShader(nullptr)
	, myDebugFragShader(nullptr)
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
	printf("[Info] OpenGL context found, version: %s\n", glGetString(GL_VERSION));
	printf("[Info] GLEW initialized, version: %s\n", glewGetString(GLEW_VERSION));
	
	// clear settings
	glClearColor(0, 0, 0, 0);
	glClearDepth(1);

	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); //set less or equal func for depth testing

	// enabling blending
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// turn on back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glViewport(0, 0, ourWidth, ourHeight);

	{
		myDebugVertShader = new ShaderGL();
		
		Shader::CreateDescriptor createDesc;
		createDesc.myType = Shader::Type::Vertex;
		myDebugVertShader->Create(createDesc);

		string contents;
		bool success = Resource::ReadFile("assets/GLShaders/debug.vert", contents);
		ASSERT_STR(success, "Failed to open debug.vert!");

		Shader::UploadDescriptor uploadDesc;
		uploadDesc.myFileContents = contents;
		success = myDebugVertShader->Upload(uploadDesc);
		ASSERT_STR(success, "Debug vertex shader failed to upload! Error: %s", 
			myDebugVertShader->GetErrorMsg().c_str());
	}
	
	{
		myDebugFragShader = new ShaderGL();

		Shader::CreateDescriptor createDesc;
		createDesc.myType = Shader::Type::Fragment;
		myDebugFragShader->Create(createDesc);

		string contents;
		bool success = Resource::ReadFile("assets/GLShaders/debug.frag", contents);
		ASSERT_STR(success, "Failed to open debug.vert!");

		Shader::UploadDescriptor uploadDesc;
		uploadDesc.myFileContents = contents;
		success = myDebugFragShader->Upload(uploadDesc);
		ASSERT_STR(success, "Debug fragment shader failed to upload! Error: %s", 
			myDebugFragShader->GetErrorMsg().c_str());
	}

	{
		myDebugPipeline = new PipelineGL();

		Pipeline::CreateDescriptor createDesc;
		myDebugPipeline->Create(createDesc);

		constexpr size_t DebugShaderCount = 2;
		const GPUResource* debugShaders[DebugShaderCount] = { myDebugFragShader, myDebugVertShader };
		Pipeline::UploadDescriptor uploadDesc;
		uploadDesc.myShaderCount = DebugShaderCount;
		uploadDesc.myShaders = debugShaders;
		myDebugPipeline->Upload(uploadDesc);
	}

	CreateLineCache();

	glActiveTexture(GL_TEXTURE0);

	ResetRenderCalls();
}

void GraphicsGL::BeginGather()
{
	// before anything, process the accumulated resource requests
	myAssetTracker.ProcessQueues();

	// reset bind state to avoid misshaps with ProcessQueues that changes OpenGL state
	myCurrentModel = nullptr;
	myCurrentPipeline = nullptr;
	myCurrentTexture = nullptr;

	// clear the buffer for next frame render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GraphicsGL::Render(const Camera& aCam, const VisualObject* aVO)
{
	ASSERT_STR(aVO, "Missing renderer!");

	// update uniforms!
	{
		UniformBlock& uniformBlock = *(aVO->GetUniforms());
		const UniformAdapter& adapter = aVO->GetUniformAdapter();
		adapter.FillUniformBlock(aCam, uniformBlock);
	}

	RenderJob job{
		aVO->GetPipeline(),
		aVO->GetTexture(),
		aVO->GetModel(),
		aVO->GetUniforms()
	};

	// we don't actually render, we just create a render job, Display() does the rendering
	{
		tbb::spin_mutex::scoped_lock spinLock(myJobsLock);
		myThreadJobs.push_back(job);
		myRenderCalls++;
	}
}

void GraphicsGL::Display()
{
	// first going to process the debug lines
	if(myLineCache.uploadDesc.myPrimitiveCount > 0)
	{
		myLineCache.myBuffer->Upload(myLineCache.uploadDesc);

		// shader first
		myDebugPipeline->Bind();
		myCurrentPipeline = myDebugPipeline;

		const GLfloat* vp = static_cast<const GLfloat*>(glm::value_ptr(myLineCache.myVp));
		glUniformMatrix4fv(0, 1, false, vp);

		// then VAO
		myLineCache.myBuffer->Bind();
		myCurrentModel = myLineCache.myBuffer;

		// now just draw em out
		GLsizei count = static_cast<GLsizei>(myLineCache.myBuffer->GetPrimitiveCount());
		glDrawArrays(myLineCache.myBuffer->GetDrawMode(), 0, count);
	}

	// TODO: remove this once we have double/tripple buffering
	tbb::spin_mutex::scoped_lock spinLock(myJobsLock);
	for (const RenderJob& r : myThreadJobs)
	{
		if (r.myPipeline.IsLastHandle()
			|| r.myModel.IsLastHandle()
			|| r.myTexture.IsLastHandle())
		{
			// one of the handles has expired, meaning noone needs it anymore
			// cheaper to skip it
			continue;
		}

		const Pipeline* pipelineRes = r.myPipeline.Get();
		PipelineGL* pipeline = static_cast<PipelineGL*>(pipelineRes->GetGPUResource_Int());
		ModelGL* model = static_cast<ModelGL*>(r.myModel->GetGPUResource_Int());
		TextureGL* texture = static_cast<TextureGL*>(r.myTexture->GetGPUResource_Int());

		if (pipeline != myCurrentPipeline)
		{
			pipeline->Bind();
			myCurrentPipeline = pipeline;
		}

		if (texture != myCurrentTexture)
		{
			texture->Bind();
			myCurrentTexture = texture;
		}

		if (model != myCurrentModel)
		{
			model->Bind();
			myCurrentModel = model;
		}

		// grabbing the descriptor because it has the locations
		// of uniform values to be uploaded to
		const Descriptor& aDesc = pipelineRes->GetDescriptor();
		size_t uniformCount = aDesc.GetUniformCount();
		// grabbign the uniform data that will be used
		const char* uniformData = r.myUniforms->GetData();
		for (size_t i = 0; i < uniformCount; i++)
		{
			const char* dataPtr = uniformData + aDesc.GetOffset(i);
			switch (aDesc.GetType(i))
			{
			case UniformType::Int:
			{
				// TODO: transition to using Uniform Blocks in shaders!
				// TODO: locations might not match order!
				int32_t data = *reinterpret_cast<const int32_t*>(dataPtr);
				glUniform1i(i, data);
				break;
			}
			case UniformType::Float:
			{
				float data = *reinterpret_cast<const float*>(dataPtr);
				glUniform1f(i, data);
				break;
			}
			case UniformType::Vec2:
			{
				const float* data = reinterpret_cast<const float*>(dataPtr);
				glUniform2f(i, *data, *(data + 1));
				break;
			}
			case UniformType::Vec3:
			{
				const float* data = reinterpret_cast<const float*>(dataPtr);
				glUniform3f(i, *data, *(data + 1), *(data + 2));
				break;
			}
			case UniformType::Vec4:
			{
				const float* data = reinterpret_cast<const float*>(dataPtr);
				glUniform4f(i, *data, *(data + 1), *(data + 2), *(data + 3));
				break;
			}
			case UniformType::Mat4:
			{
				const float* data = reinterpret_cast<const float*>(dataPtr);
				glUniformMatrix4fv(i, 1, false, data);
				break;
			}
			default:
				ASSERT(false);
			}
		}

		uint32_t drawMode = model->GetDrawMode();
		size_t primitiveCount = model->GetPrimitiveCount();
		ASSERT_STR(primitiveCount < numeric_limits<GLsizei>::max(), "Exceeded the limit of primitives!");
		glDrawElements(drawMode, static_cast<GLsizei>(primitiveCount), GL_UNSIGNED_INT, 0);
	}
	myThreadJobs.clear();
	glfwSwapBuffers(myWindow);
}

void GraphicsGL::CleanUp()
{
	myAssetTracker.UnloadAll();

	myDebugPipeline->Unload();
	delete myDebugPipeline;
	myDebugFragShader->Unload();
	delete myDebugFragShader;
	myDebugVertShader->Unload();
	delete myDebugVertShader;

	myLineCache.myBuffer->Unload();
	delete myLineCache.myBuffer;

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

void GraphicsGL::DrawLines(const Camera& aCam, const vector<PosColorVertex>& aLineCache)
{
	myLineCache.uploadDesc.myPrimitiveCount = aLineCache.size() / 2;
	myLineCache.uploadDesc.myVertices = aLineCache.data();
	myLineCache.uploadDesc.myVertCount = aLineCache.size();
	myLineCache.uploadDesc.myIndices = nullptr;
	myLineCache.uploadDesc.myIndCount = 0;

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

void GraphicsGL::OnResize(int aWidth, int aHeight)
{
	ourWidth = aWidth;
	ourHeight = aHeight;

	// TODO: move this to BeginGather to get rid of the lock
	tbb::spin_mutex::scoped_lock spinLock(myJobsLock);
	glViewport(0, 0, aWidth, aHeight);
}

void GraphicsGL::CreateLineCache()
{
	myLineCache.myBuffer = new ModelGL();
	Model::CreateDescriptor createDesc;
	createDesc.myPrimitiveType = GPUResource::Primitive::Lines;
	createDesc.myUsage = GPUResource::Usage::Dynamic;
	createDesc.myVertType = PosColorVertex::Type;
	createDesc.myIsIndexed = false;
	myLineCache.myBuffer->Create(createDesc);
}