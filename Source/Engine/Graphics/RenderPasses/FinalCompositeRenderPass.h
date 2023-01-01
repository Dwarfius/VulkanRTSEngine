#pragma once

#include <Graphics/RenderPass.h>
#include <Core/RefCounted.h>

class Pipeline;
class GPUPipeline;

class FinalCompositeRenderPass : public RenderPass
{
public:
	constexpr static Id kId = Utils::CRC32("FinalCompositeRenderPass");

	FinalCompositeRenderPass(Graphics& aGraphics, Handle<Pipeline> aPipeline);

private:
	void SubmitJobs(Graphics& aGraphics) final;
	Id GetId() const final { return kId; }
	bool HasDynamicRenderContext() const final { return true; }
	void OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;

	Handle<GPUPipeline> myPipeline;
};