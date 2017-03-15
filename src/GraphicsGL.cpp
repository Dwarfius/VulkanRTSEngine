#include "GraphicsGL.h"
#include "GameObject.h"
#include <algorithm>
#include <set>
#include <fstream>
#include <glm\gtc\type_ptr.hpp>
#include "Terrain.h"

// Public Methods
void GraphicsGL::Init(vector<Terrain> terrains)
{
	activeGraphics = this;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

	window = glfwCreateWindow(width, height, "Vulkan RTS Engine", nullptr, nullptr);
	glfwSetWindowSizeCallback(window, GraphicsGL::OnWindowResized);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	glfwMakeContextCurrent(window);
	glewExperimental = true;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		printf("[Error] GLEW failed to init, error: %s\n", glewGetErrorString(err));
		return;
	}
	printf("[Info] OpenGL context found, version: %s\n", glGetString(GL_VERSION));
	printf("[Info] GLEW initialized, version: %s\n", glewGetString(GLEW_VERSION));

	// Smooth shading
	glShadeModel(GL_SMOOTH);
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

	glViewport(0, 0, (int)width, (int)height);

	LoadResources(terrains);

	glActiveTexture(GL_TEXTURE0);
}

void GraphicsGL::BeginGather()
{
	// clear the buffer for next frame render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GraphicsGL::Render(const Camera *cam, GameObject* go, const uint32_t threadId)
{
	Renderer *r = go->GetRenderer();
	if (r == nullptr)
		return;

	vec3 scale = go->GetTransform()->GetScale();
	float maxScale = max({ scale.x, scale.y, scale.z });
	float scaledRadius = vaos[r->GetModel()].sphereRadius * maxScale;
	if (!cam->CheckSphere(go->GetTransform()->GetPos(), scaledRadius))
		return;

	RenderJob job{
		r->GetShader(),
		r->GetTexture(),
		r->GetModel(),
	};

	// copy the existing ones
	job.uniforms = go->GetUniforms();
	// add add our own
	mat4 model = job.uniforms["Model"].m;
	mat4 mvp = cam->Get() * model;

	Shader::UniformValue uniform;
	uniform.m = mvp;
	job.uniforms["mvp"] = uniform;

	// we don't actually render, we just create a render job, Display() does the rendering
	threadJobs[threadId].push_back(job);
	renderCalls[threadId]++;
}

void GraphicsGL::Display()
{
	size_t count = 0;
	for (uint i = 0; i < maxThreads; i++)
	{
		for (RenderJob r : threadJobs[i])
		{
			Model vao = vaos[r.model];
			Shader shader = shaders[r.shader];
			TextureId texture = textures[r.texture];

			if (shader.id != currShader)
			{
				glUseProgram(shader.id);
				currShader = shader.id;
			}

			if (texture != currTexture)
			{
				glBindTexture(GL_TEXTURE_2D, texture);
				glUniform1i(shader.uniforms["tex"].loc, 0);
				currTexture = texture;
			}

			if (vao.id != currModel)
			{
				glBindVertexArray(vao.id);
				currModel = vao.id;
			}

			for (auto pair = r.uniforms.begin(); pair != r.uniforms.end(); ++pair)
			{
				// check to see if shader has this uniform bind point
				auto iter = shader.uniforms.find(pair->first);
				if (iter == shader.uniforms.end())
					continue;

				// shader has it, so use it
				Shader::BindPoint bp = iter->second;
				Shader::UniformValue u = pair->second;

				// pass it's value
				switch (bp.type)
				{
				case Shader::UniformType::Int:
					glUniform1i(bp.loc, u.i);
					break;
				case Shader::UniformType::Float:
					glUniform1f(bp.loc, u.f);
					break;
				case Shader::UniformType::Vec2:
					glUniform2f(bp.loc, u.v2.x, u.v2.y);
					break;
				case Shader::UniformType::Vec3:
					glUniform3f(bp.loc, u.v3.x, u.v3.y, u.v3.z);
					break;
				case Shader::UniformType::Vec4:
					glUniform4f(bp.loc, u.v4.x, u.v4.y, u.v4.z, u.v4.w);
					break;
				case Shader::UniformType::Mat4:
					glUniformMatrix4fv(bp.loc, 1, false, (const GLfloat*)value_ptr(u.m));
					break;
				}
			}
			
			glDrawElements(GL_TRIANGLES, vao.indexCount, GL_UNSIGNED_INT, 0);
		}
		count += threadJobs[i].size();
		threadJobs[i].clear();
	}
	glfwSwapBuffers(window);
}

void GraphicsGL::CleanUp()
{
	for (Shader p : shaders)
	{
		for (uint32_t s : p.shaderSources)
			glDeleteShader(s);
		glDeleteProgram(p.id);
	}
	shaders.clear();

	for (Model m : vaos)
	{
		glDeleteBuffers(m.buffers.size(), m.buffers.data());
		glDeleteBuffers(1, &m.id);
	}
	vaos.clear();

	glDeleteTextures(textures.size(), textures.data());
	textures.clear();

	glfwDestroyWindow(window);
	activeGraphics = NULL;
}

void GraphicsGL::OnResize(int width, int height)
{
	this->width = width;
	this->height = height;
	glViewport(0, 0, width, height);
}

void GraphicsGL::OnWindowResized(GLFWwindow * window, int width, int height)
{
	if (width == 0 && height == 0)
		return;

	((GraphicsGL*)activeGraphics)->OnResize(width, height);
}

void GraphicsGL::LoadResources(vector<Terrain> terrains)
{
	// first, a couple base shaders
	for(size_t i=0; i<shadersToLoad.size(); i++)
	{
		Shader shader;

		string vert = readFile("assets/GLShaders/" + shadersToLoad[i] + ".vert");
		const char *data = vert.c_str();
		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, &data, NULL);
		glCompileShader(vertShader);

		GLint isCompiled = 0;
		glGetShaderiv(vertShader, GL_COMPILE_STATUS, &isCompiled);

		string frag = readFile("assets/GLShaders/" + shadersToLoad[i] + ".frag");
		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		data = frag.c_str();
		glShaderSource(fragShader, 1, &data, NULL);
		glCompileShader(fragShader);

		shader.id = glCreateProgram();
		glAttachShader(shader.id, vertShader);
		glAttachShader(shader.id, fragShader);
		glLinkProgram(shader.id);

		GLint isLinked = 0;
		glGetProgramiv(shader.id, GL_LINK_STATUS, &isLinked);

		if (isLinked == GL_FALSE)
		{
			GLint length = 0;
			glGetProgramiv(shader.id, GL_INFO_LOG_LENGTH, &length);

			char *msg = (char*)malloc(length);
			glGetProgramInfoLog(shader.id, length, &length, msg);

			printf("[Error] Shader linking error: %s\n", msg);
			free(msg);

			glDeleteProgram(shader.id);
			return;
		}

		//going to iterate through every uniform and cache info about it
		GLint uniformCount = 0;
		glGetProgramiv(shader.id, GL_ACTIVE_UNIFORMS, &uniformCount);
		const int maxLength = 100;
		char nameChars[maxLength];
		for (int i = 0; i < uniformCount; i++)
		{
			GLenum type = 0;
			GLsizei nameLength = 0;
			GLsizei uniSize = 0;
			glGetActiveUniform(shader.id, i, maxLength, &nameLength, &uniSize, &type, nameChars);
			string name(nameChars, nameLength);
			GLint loc = glGetUniformLocation(shader.id, name.c_str());
			
			// converting to API independent format
			Shader::UniformType uniformType;
			if (type == GL_FLOAT)
				uniformType = Shader::UniformType::Float;
			else if (type == GL_FLOAT_VEC2)
				uniformType = Shader::UniformType::Vec2;
			else if (type == GL_FLOAT_VEC3)
				uniformType = Shader::UniformType::Vec3;
			else if (type == GL_FLOAT_VEC4)
				uniformType = Shader::UniformType::Vec4;
			else if (type == GL_FLOAT_MAT4)
				uniformType = Shader::UniformType::Mat4;
			else
				uniformType = Shader::UniformType::Int;

			Shader::BindPoint bindPoint{ loc, uniformType };
			shader.uniforms[name] = bindPoint;
		}

		shader.shaderSources.push_back(fragShader);
		shader.shaderSources.push_back(vertShader);
		shaders.push_back(shader);
	}

	for(size_t i=0; i<modelsToLoad.size(); i++)
	{
		Model m;
		vector<Vertex> vertices;
		vector<uint32_t> indices;

		if (modelsToLoad[i].substr(0, 2) == "%t")
		{
			int index = stoi(modelsToLoad[i].substr(2), nullptr);
			Terrain t = terrains[index];
			vertices.insert(vertices.end(), t.GetVertBegin(), t.GetVertEnd());
			indices.insert(indices.end(), t.GetIndBegin(), t.GetIndEnd());
			m.center = t.GetCenter();
			m.sphereRadius = t.GetRange();
		}
		else
			LoadModel(modelsToLoad[i], vertices, indices, m.center, m.sphereRadius);

		m.vertexCount = vertices.size();
		m.indexCount = indices.size();
		
		glGenVertexArrays(1, &m.id);
		glBindVertexArray(m.id);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
		m.buffers.push_back(vbo);

		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * indices.size(), indices.data(), GL_STATIC_DRAW);
		m.buffers.push_back(ebo);

		// tell the VAO that 0 is the position element
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)offsetof(Vertex, pos));

		// uvs at 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)offsetof(Vertex, uv));

		// normals at 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)(offsetof(Vertex, normal)));

		glBindVertexArray(0);
		vaos.push_back(m);
	}

	// lastly, the textures
	for(size_t i=0; i<texturesToLoad.size(); i++)
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);

		int texWidth, texHeight, texChannels;
		string name = "assets/textures/" + texturesToLoad[i];
		void *pixels = LoadTexture(name, &texWidth, &texHeight, &texChannels, STBI_rgb);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		glGenerateMipmap(GL_TEXTURE_2D);

		FreeTexture(pixels);

		textures.push_back(tex);
	}
}