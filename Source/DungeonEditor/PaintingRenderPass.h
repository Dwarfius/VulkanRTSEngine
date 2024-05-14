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
	void SetPipelines(Handle<Pipeline> aPaintPipeline, 
		Handle<Pipeline> aDisplayPipeline, Graphics& aGraphics);
	void SetParams(const PaintParams& aParams);

private:
	std::string_view GetWriteBuffer() const;
	std::string_view GetReadBuffer() const;
	PaintParams GetParams() const;

	void Execute(Graphics& aGraphics) override;
	void ExecutePainting(Graphics& aGraphics);
	void ExecuteDisplay(Graphics& aGraphics);
	Id GetId() const override { return kId; };
	RenderContext CreatePaintContext(Graphics& aGraphics) const;
	RenderContext CreateDisplayContext(Graphics& aGraphics) const;

	Handle<GPUPipeline> myPaintPipeline;
	Handle<GPUPipeline> myDisplayPipeline;
	Handle<UniformBuffer> myPaintBuffer;
	Handle<UniformBuffer> myDisplayBuffer;
	PaintParams myParams;
	bool myWriteToOther = false;

#ifdef ASSERT_MUTEX
	mutable AssertMutex myParamsMutex;
#endif
};

class PainterAdapter : RegisterUniformAdapter<PainterAdapter>
{
public:
	struct Source : CameraAdapterSourceData
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

	constexpr static Descriptor ourDescriptor {
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Vec4 },
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