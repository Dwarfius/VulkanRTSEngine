#pragma once

#include <Graphics/RenderPassJob.h>

class PipelineGL;
class ModelGL;
class TextureGL;

class RenderPassJobGL : public RenderPassJob
{
public:
	void Add(const RenderJob& aJob) final;
	void AddRange(std::vector<RenderJob>&& aJobs) final;
	void Clear() final { myJobs.clear(); };
	operator std::vector<RenderJob>() && final { return myJobs; }

private:
	bool HasWork() const final;
	void OnInitialize(const RenderContext& aContext) final;
	void BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext) final;
	void Clear(const RenderContext& aContext) final;
	void SetupContext(Graphics& aGraphics, const RenderContext& aContext) final;
	void RunJobs() override final;

	constexpr static uint32_t ConvertBlendMode(RenderContext::Blending aBlendMode);
	constexpr static uint32_t ConvertBlendEquation(RenderContext::BlendingEq aBlendEq);

	std::vector<RenderJob> myJobs;
	PipelineGL* myCurrentPipeline;
	ModelGL* myCurrentModel;

	TextureGL* myCurrentTextures[RenderContext::kMaxObjectTextureSlots];
	int myTextureSlotsToUse[RenderContext::kMaxObjectTextureSlots];
	// offset where FBTextures end and Object Textures begin
	uint8_t myTextureSlotsUsed = 0;
};