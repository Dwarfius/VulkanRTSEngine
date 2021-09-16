#pragma once

#include <Graphics/RenderPass.h>
#include <Graphics/FrameBuffer.h>
#include <Core/StaticString.h>
#include <Graphics/UniformAdapter.h>

class GPUPipeline;

struct PaintingFrameBuffer
{
	constexpr static StaticString kName = "Painting";
	constexpr static FrameBuffer kDescriptor{
		{
			// Painted terrain texture
			{ FrameBuffer::AttachmentType::Texture, ITexture::Format::UNorm_RGBA },
			// composite of terrain + UI
			{ FrameBuffer::AttachmentType::Texture, ITexture::Format::UNorm_RGBA }
		}
	};
};

struct OtherPaintingFrameBuffer
{
	constexpr static StaticString kName = "OtherPainting";
	constexpr static FrameBuffer kDescriptor = PaintingFrameBuffer::kDescriptor;
};

class PaintingRenderPass : public IRenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("PaintingRenderPass");
	void SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics);

	std::string_view GetWriteBuffer() const;

private:
	void SubmitJobs(Graphics& aGraphics) final;
	Id GetId() const final { return kId; };
	bool HasDynamicRenderContext() const final { return true; }
	void PrepareContext(RenderContext& aContext) const final;

	Handle<GPUPipeline> myPipeline;
	std::shared_ptr<UniformBlock> myBlock;
	float myBrushRadius = 0.01f;
	bool myWriteToOther = false;
};

class DisplayRenderPass : public IRenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DisplayRenderPass");
	void SetPipeline(Handle<GPUPipeline> aPipeline) { myPipeline = aPipeline; }
	void SetPaintingRenderPass(PaintingRenderPass* aPass) { myPass = aPass; }

private:
	void SubmitJobs(Graphics& aGraphics) final;
	Id GetId() const final { return kId; };
	bool HasDynamicRenderContext() const final { return true; }
	void PrepareContext(RenderContext& aContext) const final;

	Handle<GPUPipeline> myPipeline;
	PaintingRenderPass* myPass;
};

class PainterAdapter : public UniformAdapter
{
	DECLARE_REGISTER(PainterAdapter);
public:
	struct Source : SourceData
	{
		const glm::vec2 myTexSize;
		const glm::vec2 myMousePos;
		const int myPaintMode;
		const float myBrushRadius;
	};

	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const override;
};