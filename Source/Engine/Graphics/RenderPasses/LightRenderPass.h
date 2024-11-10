#pragma once

#include <Graphics/RenderPass.h>
#include <Graphics/Resources/GPUBuffer.h>

class Graphics;

class LightRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("LightRenderPass");

	LightRenderPass(Graphics& aGraphics);

	Id GetId() const override { return kId; }

	void Execute(Graphics& aGraphics) override;

	std::string_view GetTypeName() const override { return "LightRenderPass"; }

private:
	Handle<GPUBuffer> myLightsUBO;
};