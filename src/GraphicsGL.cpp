#include "Graphics.h"
#include "GameObject.h"
#include <algorithm>
#include <set>
#include <fstream>
#include <glm\gtc\type_ptr.hpp>

// Public Methods
void GraphicsGL::Init()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

	window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Vulkan RTS Engine", nullptr, nullptr);
	glfwSetWindowSizeCallback(window, GraphicsGL::OnWindowResized);
	
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

	glViewport(0, 0, SCREEN_W, SCREEN_H);

	LoadResources();

	glActiveTexture(GL_TEXTURE0);
}

void GraphicsGL::Render(Camera *cam, GameObject* go, uint threadId)
{
	mat4 mvp = cam->Get() * go->GetModelMatrix();
	RenderJob job{
		go->GetShader(),
		go->GetTexture(),
		go->GetModel(),
		mvp
	};
	// we don't actually render, we just create a render job, Display() does the rendering
	threadJobs[threadId].push_back(job);
}

void GraphicsGL::Display()
{
	for (uint i = 0; i < 16; i++)
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
				glUniform1i(glGetUniformLocation(shader.id, "tex"), 0);
				currTexture = texture;
			}

			if (vao.id != currModel)
			{
				glBindVertexArray(vao.id);
				currModel = vao.id;
			}

			uint loc = glGetUniformLocation(shader.id, "mvp");
			glUniformMatrix4fv(loc, 1, false, (const GLfloat*)value_ptr(r.mvp));
			glDrawElements(GL_TRIANGLES, vao.indexCount, GL_UNSIGNED_INT, 0);
		}
		threadJobs[i].clear();
	}
	glfwSwapBuffers(window);
	// clear the buffer for next frame render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
}

void GraphicsGL::OnWindowResized(GLFWwindow * window, int width, int height)
{
	if (width == 0 && height == 0)
		return;

	glViewport(0, 0, width, height);
}

void GraphicsGL::LoadResources()
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
		shader.shaderSources.push_back(fragShader);
		shader.shaderSources.push_back(vertShader);
		shaders.push_back(shader);
	}

	for(size_t i=0; i<modelsToLoad.size(); i++)
	{
		Model m;
		vector<Vertex> vertices;
		vector<uint> indices;
		LoadModel(modelsToLoad[i], vertices, indices, m.center);

		printf("[Info] Center: %f, %f, %f\n", m.center.x, m.center.y, m.center.z);
		
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)0);

		// uvs at 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)sizeof(vec3));

		// normals at 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)(sizeof(vec3) + sizeof(vec2)));

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