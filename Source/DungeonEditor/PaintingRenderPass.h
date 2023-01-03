#pragma once

#include <Core/StaticString.h>
#include <Core/RWBuffer.h>
#include <Core/RefCounted.h>
#ifdef ASSERT_MUTEX
#include <Core/Threading/AssertMutex.h>
#endif

#include <Graphics/RenderPass.h>
#include <Graphics/FrameBuffer.h>
#include <Graphics/Camera.h>
#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>

class GPUPipeline;
class Pipeline;
class GPUTexture;
class UniformBuffer;

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
	int myBrushSize;
	float myPaintInverseScale;
};

class PaintingRenderPass final : public RenderPass
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

	void Execute(Graphics& aGraphics) override;
	Id GetId() const override { return kId; };
	bool HasDynamicRenderContext() const override { return true; }
	void OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const override;

	Handle<GPUPipeline> myPipeline;
	Handle<UniformBuffer> myBuffer;
	PaintParams myParams;
	bool myWriteToOther = false;

#ifdef ASSERT_MUTEX
	mutable AssertMutex myParamsMutex;
#endif
};

class DisplayRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DisplayRenderPass");
	void SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics);
	void SetReadBuffer(std::string_view aFrameBuffer) { myReadFrameBuffer = aFrameBuffer; }
	void SetParams(const PaintParams& aParams);

private:
	PaintParams GetParams() const;

	void Execute(Graphics& aGraphics) override;
	Id GetId() const override { return kId; };
	bool HasDynamicRenderContext() const override { return true; }
	void OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const override;

	Handle<GPUPipeline> myPipeline;
	Handle<UniformBuffer> myBuffer;
	PaintParams myParams;
	std::string_view myReadFrameBuffer;

#ifdef ASSERT_MUTEX
	mutable AssertMutex myParamsMutex;
#endif
};

class PainterAdapter : RegisterUniformAdapter<PainterAdapter>
{
public:
	struct Source : AdapterSourceData
	{
		const glm::vec3 myColor;
		const glm::vec2 myTexSize;
		const glm::vec2 myMousePosStart;
		const glm::vec2 myMousePosEnd;
		const glm::ivec2 myGridDims;
		const glm::vec2 myPaintSizeScaled;
		const PaintMode myPaintMode;
		const int myBrushRadius;
	};

	inline static const Descriptor ourDescriptor {
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Vec3 },
		{ Descriptor::UniformType::Vec2 },
		{ Descriptor::UniformType::Vec2 },
		{ Descriptor::UniformType::Vec2 },
		{ Descriptor::UniformType::Vec2 },
		{ Descriptor::UniformType::Vec2 },
		{ Descriptor::UniformType::Vec2 },
		{ Descriptor::UniformType::Int },
		{ Descriptor::UniformType::Int }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};