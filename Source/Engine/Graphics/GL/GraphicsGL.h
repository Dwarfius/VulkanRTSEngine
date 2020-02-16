#pragma once

#include <Graphics/Graphics.h>
#include <Core/RWBuffer.h>
#include <Graphics/Resources/Model.h>

class ShaderGL;
class PipelineGL;
class ModelGL;

class GraphicsGL : public Graphics
{
public:
	GraphicsGL(AssetTracker& anAssetTracker);

	void Init() override;
	void BeginGather() override;
	void Display() override;
	void CleanUp() override;

	void PrepareLineCache(size_t aCacheSize) override {}
	void RenderDebug(const Camera& aCam, const DebugDrawer& aDebugDrawer) override;

	// TODO: get rid of the static method by using a bound functor object
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);

	[[nodiscard]]
	RenderPassJob& GetRenderPassJob(uint32_t anId, const RenderContext& renderContext) override;

private:
	void OnResize(int aWidth, int aHeight);

	using RenderPassJobMap = std::unordered_map<uint32_t, RenderPassJob*>;
	RWBuffer<RenderPassJobMap, 3> myRenderPassJobs;

	// ================================================
	// Debug drawing related
	Handle<PipelineGL> myDebugPipeline;
	struct LineCache
	{
		glm::mat4 myVp;
		Model::UploadDescriptor myUploadDesc;
		Handle<ModelGL> myBuffer;
	};
	LineCache myLineCache;
	void CreateLineCache();

	// TEST - used to output number of triangles
	// generated during render calls, specifically
	// tesselation of terrain (test detalization and gpu
	// side culling)
	uint32_t myGPUQuery;
	// ======

	GPUResource* Create(Resource::Type aType) const override final;
};
