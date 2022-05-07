#pragma once

#include <Graphics/RenderPassJob.h>

class PipelineGL;
class ModelGL;
class TextureGL;

class RenderPassJobGL final : public RenderPassJob
{
	void OnInitialize(const RenderContext& aContext) override;
	void BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext) override;
	void Clear(const RenderContext& aContext) override;
	void SetupContext(Graphics& aGraphics, const RenderContext& aContext) override;
	void RunJobs(std::vector<RenderJob>& aJobs) override;

	constexpr static uint32_t ConvertBlendMode(RenderContext::Blending aBlendMode);
	constexpr static uint32_t ConvertBlendEquation(RenderContext::BlendingEq aBlendEq);

	PipelineGL* myCurrentPipeline;
	ModelGL* myCurrentModel;

	TextureGL* myCurrentTextures[RenderContext::kMaxObjectTextureSlots];
	int myTextureSlotsToUse[RenderContext::kMaxObjectTextureSlots];
	// offset where FBTextures end and Object Textures begin
	uint8_t myTextureSlotsUsed = 0;
};