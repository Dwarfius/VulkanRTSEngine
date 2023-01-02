#pragma once

#include <Graphics/RenderPass.h>

class DefaultRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DefaultRenderPass");

	struct Params
	{
		uint32_t myOffset = 0; // rendering param, offset into rendering buffer
		uint32_t myCount = -1; // rendering param, how many elements to render from a buffer, all by default
	};

	Id GetId() const override { return kId; }

	void BeginPass(Graphics& aGraphics) override;

protected:
	void OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const override;
	bool HasDynamicRenderContext() const override { return true; }
};

class TerrainRenderPass : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("TerrainRenderPass");

	struct Params
	{
		int myTileCount = 0;
	};

	TerrainRenderPass();

	Id GetId() const final { return kId; }

protected:
	void OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }
};