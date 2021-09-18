#pragma once

#include <Graphics/RenderPass.h>
#include <Core/RefCounted.h>

class Pipeline;
class GPUPipeline;

class FinalCompositeRenderPass : public IRenderPass
{
public:
	constexpr static Id kId = Utils::CRC32("FinalCompositeRenderPass");

	FinalCompositeRenderPass(Graphics& aGraphics, Handle<Pipeline> aPipeline);

private:
	void SubmitJobs(Graphics& aGraphics) final;
	Id GetId() const final { return kId; }
	bool HasDynamicRenderContext() const final { return true; }
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;

	Handle<GPUPipeline> myPipeline;
};