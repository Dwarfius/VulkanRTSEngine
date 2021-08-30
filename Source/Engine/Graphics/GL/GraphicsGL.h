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
	using Graphics::Graphics;

	void Init() final;
	void BeginGather() final;
	void Display() final;
	void CleanUp() final;

	void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer) final;
	[[nodiscard]]
	FrameBufferGL& GetFrameBufferGL(std::string_view aName);

	[[nodiscard]]
	RenderPassJob& GetRenderPassJob(IRenderPass::Id anId, const RenderContext& renderContext) final;

private:
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);
	void OnResize(int aWidth, int aHeight);

	struct IdPasJobPair
	{
		RenderPass::Id myId;
		RenderPassJob* myJob;
	};
	using RenderPassJobs = std::vector<IdPasJobPair>;
	RWBuffer<RenderPassJobs, 3> myRenderPassJobs;

	GPUResource* Create(Model*, GPUResource::UsageType aUsage) const final;
	GPUResource* Create(Pipeline*, GPUResource::UsageType aUsage) const final;
	GPUResource* Create(Texture*, GPUResource::UsageType aUsage) const final;
	GPUResource* Create(Shader*, GPUResource::UsageType aUsage) const final;

	void SortRenderPassJobs() final;

	std::unordered_map<std::string_view, FrameBufferGL> myFrameBuffers;
};
