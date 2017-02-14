#ifndef _GRAPHICS_GL_H
#define _GRAPHICS_GL_H

#include "Common.h"
#include <vector>
#include <string>

class GraphicsGL : public Graphics
{
public:
	GraphicsGL() {}

	void Init() override;
	void Render(Camera *cam, GameObject *go, uint threadId) override;
	void Display() override;
	void CleanUp() override;

	static void OnWindowResized(GLFWwindow *window, int width, int height);

	vec3 GetModelCenter(ModelId m) override { return vaos[m].center; }
private:

	vector<Shader> shaders;
	vector<TextureId> textures;
	vector<Model> vaos; //vertex array object (holds info on vbo and ebo)
	void LoadResources();

	// emulating a render queue
	// should cache the shader's uniforms as well
	struct RenderJob 
	{
		ShaderId shader;
		TextureId texture;
		ModelId model;

		unordered_map<string, Shader::UniformValue> uniforms;
	};
	// supporting max 16 threads
	// might want to multi-buffer this
	vector<RenderJob> threadJobs[16];
};

#endif // !_GRAPHICS_GL_H
