#pragma once

#include "Graphics.h"

class GraphicsGL : public Graphics
{
public:
	GraphicsGL();

	void Init(const vector<Terrain*>& aTerrainList) override;
	void BeginGather() override;
	void Render(const Camera& aCam, const GameObject* aGO) override;
	void Display() override;
	void CleanUp() override;

	void PrepareLineCache(size_t aCacheSize) override {}
	void DrawLines(const Camera& aCam, const vector<PhysicsDebugDrawer::LineDraw>& aLineCache) override;

	// TODO: get rid of the static method by using a bound functor object
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);

private:
	ModelId myCurrentModel;
	ShaderId myCurrentShader;
	TextureId myCurrentTexture;

	void OnResize(int aWidth, int aHeight);

	vector<Shader> myShaders;
	vector<TextureId> myTextures;
	void LoadResources(const vector<Terrain*>& aTerrainList);

	// emulating a render queue
	// should cache the shader's uniforms as well
	struct RenderJob 
	{
		ShaderId myShader;
		TextureId myTexture;
		ModelId myModel;

		unordered_map<string, Shader::UniformValue> myUniforms;
	};
	// TODO: replace this with a double/tripple buffer concept
	vector<RenderJob> myThreadJobs;
	tbb::spin_mutex myJobsLock;

	// Debug drawing related
	struct LineCache
	{
		size_t mySize;
		uint32_t myVao;
		uint32_t myVbo;
		glm::mat4 myVp;
	};
	LineCache myLineCache;
	void CreateLineCache();
};
