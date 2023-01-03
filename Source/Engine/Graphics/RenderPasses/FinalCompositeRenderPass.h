#pragma once

#include <Graphics/RenderPass.h>
#include <Core/RefCounted.h>

class Pipeline;
class GPUPipeline;

class FinalCompositeRenderPass final : public RenderPass
{
public:
	constexpr static Id kId = Utils::CRC32("FinalCompositeRenderPass");

	FinalCompositeRenderPass(Graphics& aGraphics, Handle<Pipeline> aPipeline);

private:
	void Execute(Graphics& aGraphics) override;
	Id GetId() const override { return kId; }
	bool HasDynamicRenderContext() const override { return true; }
	void OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const override;

	Handle<GPUPipeline> myPipeline;
};