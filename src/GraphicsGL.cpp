#include "Graphics.h"
#include "GameObject.h"
#include <algorithm>
#include <set>
#include <fstream>

GLFWwindow* Graphics::window = nullptr;
vector<ShaderId> Graphics::shaders;
vector<TextureId> Graphics::textures;
vector<ModelId> Graphics::vaos;
vector<Graphics::RenderJob> Graphics::threadJobs[16];

// Public Methods
void Graphics::Init()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

	window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Vulkan RTS Engine", nullptr, nullptr);
	glfwSetWindowSizeCallback(window, OnWindowResized);
	
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

	// fix this bit of awkwardness later
	glActiveTexture(GL_TEXTURE0);
}

void Graphics::Render(GameObject* go, uint threadId)
{
	// we don't actually render, we just create a render job, Display() does the rendering
	threadJobs[threadId].push_back(RenderJob{ go->GetShader(), go->GetTexture(), go->GetModel() });
}

void Graphics::Display()
{
	for (uint i = 0; i < 16; i++)
	{
		for (RenderJob r : threadJobs[i])
		{
			glBindTexture(GL_TEXTURE_2D, textures[r.texture]);
			glBindVertexArray(vaos[r.model]);
			glUseProgram(shaders[r.shader]);
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
		}
		threadJobs[i].clear();
	}
	glfwSwapBuffers(window);
	// clear the buffer for next frame render
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Graphics::CleanUp()
{
	glfwDestroyWindow(window);
}

void Graphics::OnWindowResized(GLFWwindow * window, int width, int height)
{
	if (width == 0 && height == 0)
		return;

	glViewport(0, 0, width, height);
}

void Graphics::LoadResources()
{
	// first, a couple base shaders
	for(size_t i=0; i<shadersToLoad.size(); i++)
	{
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

		GLuint baseProg = glCreateProgram();
		glAttachShader(baseProg, vertShader);
		glAttachShader(baseProg, fragShader);
		glLinkProgram(baseProg);

		GLint isLinked = 0;
		glGetProgramiv(baseProg, GL_LINK_STATUS, &isLinked);

		if (isLinked == GL_FALSE)
		{
			GLint length = 0;
			glGetProgramiv(baseProg, GL_INFO_LOG_LENGTH, &length);

			char *msg = (char*)malloc(length);
			glGetProgramInfoLog(baseProg, length, &length, msg);

			printf("[Error] Shader linking error: %s\n", msg);
			free(msg);

			glDeleteProgram(baseProg);
			return;
		}

		shaders.push_back(baseProg);
	}

	// now the vbos and ebos
	{
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		struct Vertex {
			vec2 pos;
			vec3 color;
		};

		Vertex vertices[3] = {
			{ vec2(0.0, -0.5), vec3(1.0, 0.0, 0.0) },
			{ vec2(0.5, 0.5),  vec3(0.0, 1.0, 0.0) },
			{ vec2(-0.5, 0.5), vec3(0.0, 0.0, 1.0) }
		};
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 3, vertices, GL_STATIC_DRAW);

		vector<uint> indices = { 0, 1, 2 };
		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * 3, indices.data(), GL_STATIC_DRAW);

		//tell the VAO that 1 is the position element
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)0);

		//tell the VAO that 1 is the color element
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)sizeof(vec2));

		glBindVertexArray(0);
		vaos.push_back(vao);
	}

	// lastly, the textures
	{
		// just a plug while NYI
		textures.push_back(0);
	}
}

string Graphics::readFile(const string & filename)
{
	ifstream file(filename, ios::ate | ios::binary);
	if (!file.is_open())
	{
		printf("[Error] Failed to open file: %s\n", filename.c_str());
		return "";
	}
	
	size_t size = file.tellg();
	string data;
	data.resize(size);
	file.seekg(0);
	file.read(&data[0], size);
	file.close();
	return data;
}