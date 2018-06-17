#pragma once

#include "Graphics.h"

class GraphicsGL : public Graphics
{
public:
	void Init(const vector<Terrain*>& terrains) override;
	void BeginGather() override;
	void Render(const Camera& cam, const GameObject *go) override;
	void Display() override;
	void CleanUp() override;

	void SetThreadingHint(glm::uint maxThreads) override {}

	static void OnWindowResized(GLFWwindow *window, int width, int height);

private:

	void OnResize(int width, int height);

	vector<Shader> shaders;
	vector<TextureId> textures;
	void LoadResources(const vector<Terrain*>& terrains);

	// emulating a render queue
	// should cache the shader's uniforms as well
	struct RenderJob 
	{
		ShaderId shader;
		TextureId texture;
		ModelId model;

		unordered_map<string, Shader::UniformValue> uniforms;
	};
	// TODO: replace this with a double/tripple buffer concept
	vector<RenderJob> threadJobs;
	tbb::spin_mutex jobsLock;
};
