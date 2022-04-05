#pragma once

#include <Graphics/RenderPassJob.h>

class PipelineGL;
class ModelGL;
class TextureGL;

class RenderPassJobGL final : public RenderPassJob
{
public:
	void Add(const RenderJob& aJob) override;
	void AddRange(std::vector<RenderJob>&& aJobs) override;
	void Clear() override { myJobs.clear(); };
	operator std::vector<RenderJob>() && override { return myJobs; }

private:
	bool HasWork() const override;
	void OnInitialize(const RenderContext& aContext) override;
	void BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext) override;
	void Clear(const RenderContext& aContext) override;
	void SetupContext(Graphics& aGraphics, const RenderContext& aContext) override;
	void RunJobs() override;

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