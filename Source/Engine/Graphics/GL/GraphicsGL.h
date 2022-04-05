#pragma once

#include <Graphics/Graphics.h>
#include <Core/RWBuffer.h>
#include <Graphics/Resources/Model.h>
#include "Graphics/GL/FrameBufferGL.h"

class PipelineGL;
class ModelGL;

class GraphicsGL final: public Graphics
{
public:
	using Graphics::Graphics;

	void Init() override;
	void Display() override;
	void EndGather() override;
	void CleanUp() override;

	void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer) override;
	void ResizeNamedFrameBuffer(std::string_view aName, glm::ivec2 aSize) override;
	[[nodiscard]]
	FrameBufferGL& GetFrameBufferGL(std::string_view aName);

	[[nodiscard]]
	RenderPassJob& GetRenderPassJob(IRenderPass::Id anId, const RenderContext& renderContext) override;

private:
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);
	void OnResize(int aWidth, int aHeight) override;

	struct IdPasJobPair
	{
		RenderPass::Id myId;
		RenderPassJob* myJob;
	};
	using RenderPassJobs = std::vector<IdPasJobPair>;
	RWBuffer<RenderPassJobs, 3> myRenderPassJobs;

	GPUResource* Create(Model*, GPUResource::UsageType aUsage) const override;
	GPUResource* Create(Pipeline*, GPUResource::UsageType aUsage) const override;
	GPUResource* Create(Texture*, GPUResource::UsageType aUsage) const override;
	GPUResource* Create(Shader*, GPUResource::UsageType aUsage) const override;

	UniformBuffer* CreateUniformBufferImpl(size_t aSize) const override;

	void SortRenderPassJobs() override;

	std::unordered_map<std::string_view, FrameBufferGL> myFrameBuffers;
	int32_t myUBOOffsetAlignment = 0;
};
