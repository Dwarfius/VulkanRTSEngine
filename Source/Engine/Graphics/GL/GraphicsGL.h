#pragma once

#include "Graphics/GL/FrameBufferGL.h"
#include "Graphics/GL/UniformBufferGL.h"
#include <Graphics/Graphics.h>
#include <Graphics/Resources/Model.h>
#include <Core/RWBuffer.h>
#include <Core/StableVector.h>

class PipelineGL;
class ModelGL;

class GraphicsGL final: public Graphics
{
public:
	GraphicsGL(AssetTracker& anAssetTracker);

	void Init() override;
	void Display() override;
	void BeginGather() override;
	void EndGather() override;
	void CleanUp() override;

	void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer) override;
	void ResizeNamedFrameBuffer(std::string_view aName, glm::ivec2 aSize) override;
	[[nodiscard]]
	FrameBufferGL& GetFrameBufferGL(std::string_view aName);

	[[nodiscard]]
	RenderPassJob& GetRenderPassJob(RenderPass::Id anId, const RenderContext& renderContext) override;

	void CleanUpUBO(UniformBuffer* aUBO) override;

private:
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);
	void OnResize(int aWidth, int aHeight) override;

	struct IdPasJobPair
	{
		RenderPass::Id myId;
		RenderPassJob* myJob;
	};
	using RenderPassJobs = std::vector<IdPasJobPair>;
	// +1 because(assuming we have 2 frames in flight)
	// while we have 1st mapped, we'll need to preserve 
	// 2nd and fill 3rd
	constexpr static uint8_t kFrames = GraphicsConfig::kMaxFramesScheduled + 1;
	RWBuffer<RenderPassJobs, kFrames> myRenderPassJobs;

	GPUResource* Create(Model*, GPUResource::UsageType aUsage) const override;
	GPUResource* Create(Pipeline*, GPUResource::UsageType aUsage) const override;
	GPUResource* Create(Texture*, GPUResource::UsageType aUsage) const override;
	GPUResource* Create(Shader*, GPUResource::UsageType aUsage) const override;

	UniformBuffer* CreateUniformBufferImpl(size_t aSize) override;

	void SortRenderPassJobs() override;

	StableVector<UniformBufferGL> myUBOs;
	std::mutex myUBOsMutex;
	constexpr static uint8_t kQueues = GraphicsConfig::kMaxFramesScheduled * 2 + 1;
	RWBuffer<tbb::concurrent_queue<UniformBufferGL*>, kQueues> myUBOCleanUpQueues;
	std::unordered_map<std::string_view, FrameBufferGL> myFrameBuffers;
	int32_t myUBOOffsetAlignment = 0;
	uint8_t myWriteFenceInd = GraphicsConfig::kMaxFramesScheduled;
	uint8_t myReadFenceInd = 0;
	void* myFrameFences[3]{ nullptr };
};
