#pragma once

#include <Graphics/RenderPass.h>

class DefaultRenderPass : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DefaultRenderPass");

	struct Params
	{
		uint32_t myOffset = 0; // rendering param, offset into rendering buffer
		uint32_t myCount = -1; // rendering param, how many elements to render from a buffer, all by default
	};

	DefaultRenderPass();

	Id GetId() const final { return kId; }

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }
};

class TerrainRenderPass : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("TerrainRenderPass");

	struct Params
	{
		int myTileCount = 0;
	};

	Id GetId() const final { return kId; }

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }
};