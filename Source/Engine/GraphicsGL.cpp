#include "Common.h"
#include "GraphicsGL.h"
#include "GameObject.h"
#include "Camera.h"
#include "Terrain.h"

#include "Components/Renderer.h"

#define CHECK_GL()															\
{																			\
	const GLenum err = glGetError();										\
	if (err != GL_NO_ERROR)													\
		printf("GL error(%d) on line %d in %s", err, __LINE__, __FILE__);	\
}

GraphicsGL::GraphicsGL()
	: myCurrentModel(0)
	, myCurrentShader(0)
	, myCurrentTexture(0)
{
	memset(&myLineCache, 0, sizeof(myLineCache));
}

void GraphicsGL::Init(const vector<Terrain*>& aTerrainList)
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

	//enabling blending
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// turn on back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glViewport(0, 0, ourWidth, ourHeight);

	LoadResources(aTerrainList);

	CreateLineCache();

	glActiveTexture(GL_TEXTURE0);

	ResetRenderCalls();
}

void GraphicsGL::BeginGather()
{
	// clear the buffer for next frame render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GraphicsGL::Render(const Camera& aCam, const GameObject* aGO)
{
	const Renderer* r = aGO->GetRenderer();
	assert(r);

	RenderJob job{
		r->GetShader(),
		r->GetTexture(),
		r->GetModel(),
	};

	// copy the existing ones
	job.myUniforms = aGO->GetUniforms();
	// add add our own
	glm::mat4 model = aGO->GetMatrix();
	glm::mat4 mvp = aCam.Get() * model;

	Shader::UniformValue uniform;
	uniform.myMatrix = mvp;
	job.myUniforms["mvp"] = uniform;

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
	if(myLineCache.mySize > 0)
	{
		// shader first
		Shader debugShader = myShaders[DebugShaderInd];
		glUseProgram(debugShader.myId);
		const GLfloat* lineCache = static_cast<const GLfloat*>(glm::value_ptr(myLineCache.myVp));
		glUniformMatrix4fv(debugShader.myUniforms.begin()->second.myLocation, 1, false, lineCache);
		myCurrentShader = debugShader.myId;

		// then VAO
		glBindVertexArray(myLineCache.myVao);
		myCurrentModel = myLineCache.myVao;

		// now just draw em out
		GLsizei count = static_cast<GLsizei>(2 * myLineCache.mySize);
		glDrawArrays(GL_LINES, 0, count);
	}

	// TODO: remove this once we have double/tripple buffering
	tbb::spin_mutex::scoped_lock spinLock(myJobsLock);
	for (const RenderJob& r : myThreadJobs)
	{
		Model vao = myModels[r.myModel];
		Shader shader = myShaders[r.myShader];
		TextureId texture = myTextures[r.myTexture];

		if (shader.myId != myCurrentShader)
		{
			glUseProgram(shader.myId);
			myCurrentShader = shader.myId;
		}

		if (texture != myCurrentTexture)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			glUniform1i(shader.myUniforms["tex"].myLocation, 0);
			myCurrentTexture = texture;
		}

		if (vao.myId != myCurrentModel)
		{
			glBindVertexArray(vao.myId);
			myCurrentModel = vao.myId;
		}

		for (const pair<string, Shader::UniformValue>& pair : r.myUniforms)
		{
			// check to see if shader has this uniform bind point
			auto iter = shader.myUniforms.find(pair.first);
			if (iter == shader.myUniforms.end())
			{
				continue;
			}

			// shader has it, so use it
			Shader::BindPoint bp = iter->second;
			Shader::UniformValue u = pair.second;

			// pass it's value
			switch (bp.myType)
			{
			case Shader::UniformType::Int:
				glUniform1i(bp.myLocation, u.myInt);
				break;
			case Shader::UniformType::Float:
				glUniform1f(bp.myLocation, u.myFloat);
				break;
			case Shader::UniformType::Vec2:
				glUniform2f(bp.myLocation, u.myV2.x, u.myV2.y);
				break;
			case Shader::UniformType::Vec3:
				glUniform3f(bp.myLocation, u.myV3.x, u.myV3.y, u.myV3.z);
				break;
			case Shader::UniformType::Vec4:
				glUniform4f(bp.myLocation, u.myV4.x, u.myV4.y, u.myV4.z, u.myV4.w);
				break;
			case Shader::UniformType::Mat4:
				glUniformMatrix4fv(bp.myLocation, 1, false, (const GLfloat*)glm::value_ptr(u.myMatrix));
				break;
			}
		}
			
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vao.myIndexCount), GL_UNSIGNED_INT, 0);
	}
	myThreadJobs.clear();
	glfwSwapBuffers(myWindow);
}

void GraphicsGL::CleanUp()
{
	for (Shader p : myShaders)
	{
		for (uint32_t s : p.myGLSources)
		{
			glDeleteShader(s);
		}
		glDeleteProgram(p.myId);
	}
	myShaders.clear();

	for (Model m : myModels)
	{
		constexpr size_t elemCount = sizeof(m.myGLBuffers) / sizeof(uint32_t);
		glDeleteBuffers(elemCount, m.myGLBuffers);
		glDeleteBuffers(1, &m.myId);
	}
	myModels.clear();

	glDeleteBuffers(1, &myLineCache.myVbo);
	glDeleteBuffers(1, &myLineCache.myVao);

	glDeleteTextures(static_cast<GLsizei>(myTextures.size()), myTextures.data());
	myTextures.clear();

	glfwDestroyWindow(myWindow);
	ourActiveGraphics = nullptr;
}

void GraphicsGL::DrawLines(const Camera& aCam, const vector<Graphics::LineDraw>& aLineCache)
{
	glBufferData(GL_ARRAY_BUFFER, sizeof(Graphics::LineDraw) * aLineCache.size(), aLineCache.data(), GL_DYNAMIC_DRAW);
	myLineCache.mySize = aLineCache.size();
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

void GraphicsGL::LoadResources(const vector<Terrain*>& aTerrainList)
{
	// first, a couple base shaders
	for(size_t i=0; i<ourShadersToLoad.size(); i++)
	{
		Shader shader;

		string vert = ReadFile("assets/GLShaders/" + ourShadersToLoad[i] + ".vert");
		const char *data = vert.c_str();
		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, &data, NULL);
		glCompileShader(vertShader);

		GLint isCompiled = 0;
		glGetShaderiv(vertShader, GL_COMPILE_STATUS, &isCompiled);

		string frag = ReadFile("assets/GLShaders/" + ourShadersToLoad[i] + ".frag");
		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		data = frag.c_str();
		glShaderSource(fragShader, 1, &data, NULL);
		glCompileShader(fragShader);

		shader.myId = glCreateProgram();
		glAttachShader(shader.myId, vertShader);
		glAttachShader(shader.myId, fragShader);
		glLinkProgram(shader.myId);

		GLint isLinked = 0;
		glGetProgramiv(shader.myId, GL_LINK_STATUS, &isLinked);

		if (isLinked == GL_FALSE)
		{
			GLint length = 0;
			glGetProgramiv(shader.myId, GL_INFO_LOG_LENGTH, &length);

			char *msg = (char*)malloc(length);
			glGetProgramInfoLog(shader.myId, length, &length, msg);

			printf("[Error] Shader linking error: %s\n", msg);
			free(msg);

			glDeleteProgram(shader.myId);
			return;
		}

		//going to iterate through every uniform and cache info about it
		GLint uniformCount = 0;
		glGetProgramiv(shader.myId, GL_ACTIVE_UNIFORMS, &uniformCount);
		const int maxLength = 100;
		char nameChars[maxLength];
		for (int i = 0; i < uniformCount; i++)
		{
			GLenum type = 0;
			GLsizei nameLength = 0;
			GLsizei uniSize = 0;
			glGetActiveUniform(shader.myId, i, maxLength, &nameLength, &uniSize, &type, nameChars);
			string name(nameChars, nameLength);
			GLint loc = glGetUniformLocation(shader.myId, name.c_str());
			
			// converting to API independent format
			Shader::UniformType uniformType;
			switch (type)
			{
			case GL_FLOAT:		uniformType = Shader::UniformType::Float;	break;
			case GL_FLOAT_VEC2:	uniformType = Shader::UniformType::Vec2;	break;
			case GL_FLOAT_VEC3: uniformType = Shader::UniformType::Vec3;	break;
			case GL_FLOAT_VEC4: uniformType = Shader::UniformType::Vec4;	break;
			case GL_FLOAT_MAT4: uniformType = Shader::UniformType::Mat4;	break;
			default:			uniformType = Shader::UniformType::Int;		break;
			}

			Shader::BindPoint bindPoint{ loc, uniformType };
			shader.myUniforms[name] = bindPoint;
		}

		// TODO: replace 0 and 1 with ShaderType enum
		shader.myGLSources[0] = fragShader;
		shader.myGLSources[1] = vertShader;
		myShaders.push_back(shader);
	}

	for(size_t i=0; i<ourModelsToLoad.size(); i++)
	{
		Model m;
		vector<Vertex> vertices;
		vector<IndexType> indices;

		if (ourModelsToLoad[i].substr(0, 2) == "%t")
		{
			int index = stoi(ourModelsToLoad[i].substr(2), nullptr);
			const Terrain& t = *aTerrainList[index];
			vertices.insert(vertices.end(), t.GetVertBegin(), t.GetVertEnd());
			indices.insert(indices.end(), t.GetIndBegin(), t.GetIndEnd());
			m.myCenter = t.GetCenter();
			m.mySphereRadius = t.GetRange();
		}
		else
		{
			LoadModel(ourModelsToLoad[i], vertices, indices, m.myCenter, m.mySphereRadius);
		}

		m.myVertexCount = vertices.size();
		m.myIndexCount = indices.size();
		
		glGenVertexArrays(1, &m.myId);
		glBindVertexArray(m.myId);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
		// TODO: replace 0 with BufferType enum
		m.myGLBuffers[0] = vbo;

		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * indices.size(), indices.data(), GL_STATIC_DRAW);
		// TODO: replace 1 with BufferType enum
		m.myGLBuffers[1] = ebo;

		// tell the VAO that 0 is the position element
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)offsetof(Vertex, myPos));

		// uvs at 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)offsetof(Vertex, myUv));

		// normals at 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)(offsetof(Vertex, myNormal)));

		// we're done setting up the VBA, so unbind all resources
		glBindVertexArray(0);
		myModels.push_back(m);
	}

	// lastly, the textures
	for(size_t i=0; i<ourTexturesToLoad.size(); i++)
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int texWidth, texHeight, texChannels;
		string name = "assets/textures/" + ourTexturesToLoad[i];
		void *pixels = LoadTexture(name, &texWidth, &texHeight, &texChannels, STBI_rgb);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		// TODO: enable mipmap generation later
		//glGenerateMipmap(GL_TEXTURE_2D);

		FreeTexture(pixels);

		myTextures.push_back(tex);
	}
}

void GraphicsGL::CreateLineCache()
{
	glGenVertexArrays(1, &myLineCache.myVao);
	glBindVertexArray(myLineCache.myVao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	myLineCache.myVbo = vbo;

	// tell the VAO that 0 is the position element
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosColorVertex), (void**)offsetof(PosColorVertex, myPos));

	// color at 1
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PosColorVertex), (void**)offsetof(PosColorVertex, myColor));

	// we're done setting up the VBA, so unbind all resources
	glBindVertexArray(0);
}