#pragma once

#include <Graphics/Graphics.h>
#include <Core/RWBuffer.h>
#include <Graphics/Resources/Model.h>
#include "Graphics/GL/FrameBufferGL.h"

class PipelineGL;
class ModelGL;

class GraphicsGL : public Graphics
{
public:
	GraphicsGL(AssetTracker& anAssetTracker);

	void Init() final;
	void BeginGather() final;
	void Display() final;
	void CleanUp() final;

	void PrepareLineCache(size_t aCacheSize) final {}
	void RenderDebug(const Camera& aCam, const DebugDrawer& aDebugDrawer) final;

	// TODO: get rid of the static method by using a bound functor object
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);

	void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer) final;
	[[nodiscard]]
	FrameBufferGL& GetFrameBufferGL(std::string_view aName);

	[[nodiscard]]
	RenderPassJob& GetRenderPassJob(uint32_t anId, const RenderContext& renderContext) final;

private:
	void OnResize(int aWidth, int aHeight);

	using RenderPassJobMap = std::unordered_map<uint32_t, RenderPassJob*>;
	RWBuffer<RenderPassJobMap, 3> myRenderPassJobs;

	// ================================================
	// Debug drawing related
	Handle<PipelineGL> myDebugPipeline;
	struct LineCache
	{
		using VertType = PosColorVertex;
		using UploadDesc = Model::UploadDescriptor<VertType>;

		glm::mat4 myVp;
		UploadDesc myUploadDesc;
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

	GPUResource* Create(Model*) const final;
	GPUResource* Create(Pipeline*) const final;
	GPUResource* Create(Texture*) const final;
	GPUResource* Create(Shader*) const final;

	std::unordered_map<std::string_view, FrameBufferGL> myFrameBuffers;
	Handle<ModelGL> myFrameQuad;
	Handle<PipelineGL> myCompositePipeline;
	void CreateFrameQuad();
};
