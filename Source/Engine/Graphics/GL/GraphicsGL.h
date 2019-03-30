#pragma once

#include <Core/Graphics/Graphics.h>
#include "ModelGL.h"
#include "PipelineGL.h"
#include "TextureGL.h"

class ShaderGL;

class GraphicsGL : public Graphics
{
public:
	GraphicsGL(AssetTracker& anAssetTracker);

	void Init() override;
	void BeginGather() override;
	void Render(const Camera& aCam, const VisualObject* aVO) override;
	void Display() override;
	void CleanUp() override;

	GPUResource* Create(Resource::Type aType) const override;

	void PrepareLineCache(size_t aCacheSize) override {}
	void DrawLines(const Camera& aCam, const vector<PosColorVertex>& aLineCache) override;

	// TODO: get rid of the static method by using a bound functor object
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);

private:
	ModelGL* myCurrentModel;
	PipelineGL* myCurrentPipeline;
	TextureGL* myCurrentTexture;

	void OnResize(int aWidth, int aHeight);

	// emulating a render queue
	// should cache the shader's uniforms as well
	struct RenderJob 
	{
		Handle<Pipeline> myPipeline;
		Handle<Texture> myTexture;
		Handle<Model> myModel;

		vector<shared_ptr<UniformBlock>> myUniforms;
	};
	// TODO: replace this with a double/tripple buffer concept
	vector<RenderJob> myThreadJobs;
	tbb::spin_mutex myJobsLock;

	// ================================================
	// Debug drawing related
	ShaderGL* myDebugVertShader;
	ShaderGL* myDebugFragShader;
	PipelineGL* myDebugPipeline;
	struct LineCache
	{
		ModelGL* myBuffer;
		glm::mat4 myVp;
		Model::UploadDescriptor uploadDesc;
	};
	LineCache myLineCache;
	void CreateLineCache();
};
