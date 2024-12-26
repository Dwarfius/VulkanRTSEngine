#pragma once

#include "Graphics/GL/FrameBufferGL.h"
#include "Graphics/GL/GPUBufferGL.h"
#include "Graphics/GL/RenderPassJobGL.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/Model.h>
#include <Core/RWBuffer.h>
#include <Core/StableVector.h>

// TODO: make StableVector work with incomplete types -
// It's currently needed because of by-value start page
#include "Graphics/GL/PipelineGL.h"
#include "Graphics/GL/ModelGL.h"
#include "Graphics/GL/TextureGL.h"

class PipelineGL;
class ModelGL;
class TextureGL;
class ShaderGL;

class GraphicsGL final: public Graphics
{
public:
	GraphicsGL(AssetTracker& anAssetTracker);

	void Init() override;
	void Display() override;
	void Gather() override;
	void CleanUp() override;

	void AddNamedFrameBuffer(std::string_view aName, const FrameBuffer& aBvuffer) override;
	void ResizeNamedFrameBuffer(std::string_view aName, glm::ivec2 aSize) override;
	[[nodiscard]]
	FrameBufferGL& GetFrameBufferGL(std::string_view aName);

	[[nodiscard]]
	RenderPassJob& CreateRenderPassJob(const RenderContext& renderContext) override;

	void CleanUpUBO(GPUBuffer* aUBO) override;

	std::string_view GetTypeName() const override { return "GraphicsGL"; }

private:
	static void OnWindowResized(GLFWwindow* aWindow, int aWidth, int aHeight);
	void OnResize(int aWidth, int aHeight) override;

	void DeleteResource(GPUResource* aResource) override;

	constexpr static uint32_t kMaxRenderPassJobs = 128;
	struct RenderPassJobs
	{
		std::array<RenderPassJobGL, kMaxRenderPassJobs> myJobs;
		std::atomic<uint32_t> myJobCounter = 0;
	};
	// +1 because(assuming we have 2 frames in flight)
	// while we have 1st mapped, we'll need to preserve 
	// 2nd and fill 3rd
	constexpr static uint8_t kFrames = GraphicsConfig::kMaxFramesScheduled + 1;
	RWBuffer<RenderPassJobs, kFrames> myRenderPassJobs;

	GPUResource* Create(Model*, GPUResource::UsageType aUsage) override;
	GPUResource* Create(Pipeline*, GPUResource::UsageType aUsage) override;
	GPUResource* Create(Texture*, GPUResource::UsageType aUsage) override;
	GPUResource* Create(Shader*, GPUResource::UsageType aUsage) override;

	StableVector<ModelGL> myModels;
	StableVector<PipelineGL> myPipelines;
	StableVector<TextureGL> myTextures;
	std::mutex myModelsMutex;
	std::mutex myPipelinesMutex;
	std::mutex myTexturesMutex;

	GPUBuffer* CreateUniformBufferImpl(size_t aSize) override;
	GPUBuffer* CreateShaderStorageBufferImpl(size_t aSize) override;

	StableVector<GPUBufferGL> myUBOs;
	std::mutex myUBOsMutex;
	StableVector<GPUBufferGL> mySSBOs;
	std::mutex mySSBOsMutex;
	constexpr static uint8_t kQueues = GraphicsConfig::kMaxFramesScheduled * 2 + 1;
	RWBuffer<tbb::concurrent_queue<GPUBufferGL*>, kQueues> myUBOCleanUpQueues;
	std::unordered_map<std::string_view, FrameBufferGL> myFrameBuffers;
	int32_t myUBOOffsetAlignment = 0;
	uint8_t myWriteFenceInd = GraphicsConfig::kMaxFramesScheduled;
	uint8_t myReadFenceInd = 0;
	void* myFrameFences[3]{ nullptr };
};
