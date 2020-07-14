#pragma once

#include <Graphics/RenderPass.h>

// TODO: test SortingRenderPass
class DefaultRenderPass : public RenderPass
{
	constexpr static uint32_t PassId = Utils::CRC32("DefaultRenderPass");

public:
	bool HasResources(const RenderJob& aJob) const override final;
	uint32_t Id() const override { return PassId; }

protected:
	void PrepareContext(RenderContext& aContext) const override;
	Category GetCategory() const override { return Category::Renderables; }
	void Process(RenderJob& aJob, const IParams& aParams) const override;
};

struct TerrainRenderParams : public IRenderPass::IParams
{
	int myTileCount = 0;
};

class TerrainRenderPass : public RenderPass
{
	constexpr static uint32_t PassId = Utils::CRC32("TerrainRenderPass");

public:
	bool HasResources(const RenderJob& aJob) const override final;
	uint32_t Id() const override { return PassId; }

protected:
	void PrepareContext(RenderContext& aContext) const override;
	Category GetCategory() const override { return Category::Terrain; }
	void Process(RenderJob& aJob, const IParams& aParams) const override;
	bool HasDynamicRenderContext() const override { return true; }
};