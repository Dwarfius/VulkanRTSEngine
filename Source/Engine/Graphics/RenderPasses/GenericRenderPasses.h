#pragma once

#include <Graphics/RenderPass.h>

class DefaultRenderPass : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DefaultRenderPass");
	DefaultRenderPass();

	Id GetId() const final { return kId; }

	void Process(RenderJob& aJob, const IParams& aParams) const final;

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }
};

struct TerrainRenderParams : public IRenderPass::IParams
{
	int myTileCount = 0;
};

class TerrainRenderPass : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("TerrainRenderPass");

	Id GetId() const final { return kId; }

	void Process(RenderJob& aJob, const IParams& aParams) const final;

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }
};