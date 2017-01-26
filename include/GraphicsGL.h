#ifndef _GRAPHICS_GL_H
#define _GRAPHICS_GL_H

#include "Common.h"
#include <vector>
#include <string>

class Graphics
{
public:
	static void Init();
	static void Render(GameObject* go, uint threadId);
	static void Display();
	static void CleanUp();

	static GLFWwindow* GetWindow() { return window; }

	static void OnWindowResized(GLFWwindow *window, int width, int height);
private:
	Graphics();

	static GLFWwindow *window;

	static vector<ShaderId> shaders;
	static vector<TextureId> textures;
	static vector<ModelId> vaos; //vertex array object (holds info on vbo and ebo)
	static void LoadResources();

	// emulating a render queue
	// should cache the shader's uniforms as well
	struct RenderJob {
		ShaderId shader;
		TextureId texture;
		ModelId model;
	};
	// supporting max 16 threads
	// might want to multi-buffer this
	static vector<RenderJob> threadJobs[16];

	// Utilities
	static string readFile(const string& filename);
};

#endif // !_GRAPHICS_GL_H
