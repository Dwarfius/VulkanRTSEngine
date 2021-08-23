#pragma once

#include <Graphics/RenderPass.h>

// TODO: test SortingRenderPass
class DefaultRenderPass : public RenderPass
{
public:
	constexpr static uint32_t PassId = Utils::CRC32("DefaultRenderPass");
	DefaultRenderPass();

	bool HasResources(const RenderJob& aJob) const final;
	uint32_t Id() const final { return PassId; }

protected:
	void PrepareContext(RenderContext& aContext) const final;
	Category GetCategory() const final { return Category::Renderables; }
	void Process(RenderJob& aJob, const IParams& aParams) const final;
	bool HasDynamicRenderContext() const final { return true; }
};

struct TerrainRenderParams : public IRenderPass::IParams
{
	int myTileCount = 0;
};

class TerrainRenderPass : public RenderPass
{
public:
	constexpr static uint32_t PassId = Utils::CRC32("TerrainRenderPass");
	bool HasResources(const RenderJob& aJob) const final;
	uint32_t Id() const final { return PassId; }

protected:
	void PrepareContext(RenderContext& aContext) const final;
	Category GetCategory() const final { return Category::Terrain; }
	void Process(RenderJob& aJob, const IParams& aParams) const final;
	bool HasDynamicRenderContext() const final { return true; }
};