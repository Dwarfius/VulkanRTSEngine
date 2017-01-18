#ifndef _GRAPHICS_GL_H
#define _GRAPHICS_GL_H

#include "Common.h"
#include <vector>
#include <string>

class Graphics
{
public:
	static void Init();
	static void Render(GameObject* go);
	static void Display();
	static void CleanUp();

	static GLFWwindow* GetWindow() { return window; }

	static void OnWindowResized(GLFWwindow *window, int width, int height);
private:
	Graphics();

	static GLFWwindow *window;

	static vector<GLuint> shaders;
	static vector<GLuint> textures;
	static vector<GLuint> vaos; //vertex array object (holds info on vbo and ebo)
	static void LoadResources();

	// Utilities
	static string readFile(const string& filename);
};

#endif // !_GRAPHICS_GL_H
