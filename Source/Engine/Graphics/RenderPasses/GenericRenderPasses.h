#pragma once

#include <Graphics/RenderPass.h>

class DefaultRenderPass : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DefaultRenderPass");
	DefaultRenderPass();

	bool HasResources(const RenderJob& aJob) const;
	Id GetId() const final { return kId; }

protected:
	void PrepareContext(RenderContext& aContext) const final;
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
	constexpr static uint32_t kId = Utils::CRC32("TerrainRenderPass");

	bool HasResources(const RenderJob& aJob) const;
	Id GetId() const final { return kId; }

protected:
	void PrepareContext(RenderContext& aContext) const final;
	void Process(RenderJob& aJob, const IParams& aParams) const final;
	bool HasDynamicRenderContext() const final { return true; }
};