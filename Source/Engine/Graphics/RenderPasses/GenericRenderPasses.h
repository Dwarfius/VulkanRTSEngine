#pragma once

#include <Graphics/RenderPass.h>

// TODO: test SortingRenderPass
class DefaultRenderPass : public RenderPass
{
protected:
	uint32_t Id() const override { return Utils::CRC32("DefaultRenderPass"); }
	void PrepareContext(RenderContext& aContext) const override;
	Category GetCategory() const override { return Category::Renderables; }
	void Process(RenderJob& aJob, const IParams& aParams) const override;
};

struct TerrainRenderParams : public IRenderPass::IParams
{
	int myTileCount;
};

class TerrainRenderPass : public RenderPass
{
protected:
	uint32_t Id() const override { return Utils::CRC32("TerrainRenderPass"); }
	void PrepareContext(RenderContext& aContext) const override;
	Category GetCategory() const override { return Category::Terrain; }
	void Process(RenderJob& aJob, const IParams& aParams) const override;
	bool HasDynamicRenderContext() const override { return true; }
};