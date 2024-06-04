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

	void Execute(Graphics& aGraphics) override;

	std::string_view GetTypeName() const override { return "DefaultRenderPass"; }

private:
	RenderContext CreateContext(Graphics& aGraphics) const;
};

class TerrainRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("TerrainRenderPass");

	struct Params
	{
		int myTileCount = 0;
	};

	TerrainRenderPass();

	Id GetId() const override { return kId; }

	void Execute(Graphics& aGraphics) override;

	std::string_view GetTypeName() const override { return "TerrainRenderPass"; }

private:
	RenderContext CreateContext(Graphics& aGraphics) const;
};