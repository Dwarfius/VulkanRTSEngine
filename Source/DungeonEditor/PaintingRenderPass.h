#pragma once

#include <Graphics/RenderPass.h>
#include <Graphics/FrameBuffer.h>
#include <Core/StaticString.h>
#include <Graphics/UniformAdapter.h>
#include <Graphics/Camera.h>
#include <Core/RWBuffer.h>

class GPUPipeline;
class Pipeline;
class GPUTexture;

struct PaintingFrameBuffer
{
	constexpr static StaticString kName = "Painting";
	constexpr static FrameBuffer kDescriptor{
		{
			// Painted terrain texture
			{ FrameBuffer::AttachmentType::Texture, ITexture::Format::UNorm_RGBA },
			// composite of terrain + UI
			{ FrameBuffer::AttachmentType::Texture, ITexture::Format::UNorm_RGBA }
		},
		{},
		{},
		glm::ivec2{800, 600}
	};
	constexpr static uint8_t kFinalColor = 0;
	constexpr static uint8_t kPaintingColor = 1;
};

struct OtherPaintingFrameBuffer
{
	constexpr static StaticString kName = "OtherPainting";
	constexpr static FrameBuffer kDescriptor = PaintingFrameBuffer::kDescriptor;
};

enum class PaintMode : int
{
	None = 0,
	PaintColor = 1,
	PaintTexture = 2,
	EraseAll = -1,
	ErasePtr = -2
};

struct PaintParams
{
	Handle<GPUTexture> myPaintTexture;
	Camera myCamera;
	glm::vec3 myColor;
	glm::vec2 myTexSize;
	glm::vec2 myPrevMousePos;
	glm::vec2 myMousePos;
	glm::ivec2 myGridDims;
	PaintMode myPaintMode;
	float myBrushSize;
	float myPaintInverseScale;
};

class PaintingRenderPass : public IRenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("PaintingRenderPass");
	
public:
	void SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics);
	void SetParams(const PaintParams& aParams);

private:
	std::string_view GetWriteBuffer() const;
	std::string_view GetReadBuffer() const;
	PaintParams GetParams() const;

	void SubmitJobs(Graphics& aGraphics) final;
	Id GetId() const final { return kId; };
	bool HasDynamicRenderContext() const final { return true; }
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;

	Handle<GPUPipeline> myPipeline;
	std::shared_ptr<UniformBlock> myBlock;
	PaintParams myParams;
	bool myWriteToOther = false;

#ifdef ASSERT_MUTEX
	mutable AssertMutex myParamsMutex;
#endif
};

class DisplayRenderPass : public IRenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DisplayRenderPass");
	void SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics);
	void SetReadBuffer(std::string_view aFrameBuffer) { myReadFrameBuffer = aFrameBuffer; }
	void SetParams(const PaintParams& aParams);

private:
	PaintParams GetParams() const;

	void SubmitJobs(Graphics& aGraphics) final;
	Id GetId() const final { return kId; };
	bool HasDynamicRenderContext() const final { return true; }
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;

	Handle<GPUPipeline> myPipeline;
	std::shared_ptr<UniformBlock> myBlock;
	PaintParams myParams;
	std::string_view myReadFrameBuffer;

#ifdef ASSERT_MUTEX
	mutable AssertMutex myParamsMutex;
#endif
};

class PainterAdapter : public UniformAdapter
{
	DECLARE_REGISTER(PainterAdapter);
public:
	struct Source : SourceData
	{
		const glm::vec3 myColor;
		const glm::vec2 myTexSize;
		const glm::vec2 myMousePosStart;
		const glm::vec2 myMousePosEnd;
		const glm::ivec2 myGridDims;
		const glm::vec2 myPaintSizeScaled;
		const PaintMode myPaintMode;
		const float myBrushRadius;
	};

	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const override;
};