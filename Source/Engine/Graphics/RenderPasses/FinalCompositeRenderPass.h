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

	std::string_view GetTypeName() const override { return "FinalCompositeRenderPass"; }

private:
	void Execute(Graphics& aGraphics) override;
	Id GetId() const override { return kId; }
	RenderContext CreateContext(Graphics& aGraphics) const;

	Handle<GPUPipeline> myPipeline;
};