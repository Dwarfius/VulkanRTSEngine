#pragma once

#include <Graphics/RenderPass.h>

#include <Core/RefCounted.h>

class GPUPipeline;
class GPUBuffer;
class Graphics;

class GrassRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("GrassRenderPass");
	GrassRenderPass(Graphics& aGraphics);
	
	void Execute(Graphics& aGraphics) override;
	Id GetId() const override { return kId; }
	std::string_view GetTypeName() const override { return "GrassRenderPass"; }

private:
	Handle<GPUPipeline> myComputePipeline;
	Handle<GPUBuffer> myCamBuffer;
	Handle<GPUBuffer> myPosBuffer;
};