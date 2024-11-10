#pragma once

#include <Graphics/RenderPass.h>

class DefaultRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DefaultRenderPass");

	DefaultRenderPass();

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

	TerrainRenderPass();

	Id GetId() const override { return kId; }

	void Execute(Graphics& aGraphics) override;

	std::string_view GetTypeName() const override { return "TerrainRenderPass"; }

private:
	RenderContext CreateContext(Graphics& aGraphics) const;
};