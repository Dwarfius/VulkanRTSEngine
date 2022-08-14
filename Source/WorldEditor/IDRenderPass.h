#pragma once

#include <Graphics/FrameBuffer.h>
#include <Graphics/RenderPass.h>
#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>
#include <Core/RefCounted.h>
#include <Core/StaticString.h>
#include <Core/RWBuffer.h>

struct Renderable;

class IDRenderPass : public RenderPass
{
public:
	using ObjID = uint32_t;
	static constexpr ObjID kMaxObjects = 1024;

public:
	constexpr static uint32_t kId = Utils::CRC32("IDRenderPass");
	IDRenderPass(Graphics& aGraphics, 
		const Handle<GPUPipeline>& aDefaultPipeline,
		const Handle<GPUPipeline>& aSkinningPipeline
	);

	Id GetId() const final { return kId; }

	void BeginPass(Graphics& aGraphics) override;
	void ScheduleRenderable(Graphics& aGraphics, Renderable& aRenderable, Camera& aCamera);
	void Process(RenderJob& aJob, const IParams& aParams) const final {}

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }

	struct ObjIDPair
	{
		GameObject* myGO;
		ObjID myID;
	};
	struct FrameObjs
	{
		std::array<GameObject*, kMaxObjects> myGOs;
		std::atomic<ObjID> myCounter = 0;
	};
	RWBuffer<FrameObjs, 3> myFrameGOs;
	Handle<GPUPipeline> myDefaultPipeline;
	Handle<GPUPipeline> mySkinningPipeline;
};

// the engine provides a default render buffer, that
// generic render passes output to
struct IDFrameBuffer
{
	constexpr static StaticString kName = "IDs";
	constexpr static FrameBuffer kDescriptor{
		{
			{ FrameBuffer::AttachmentType::Texture, ITexture::Format::U_R }
		},
		{ FrameBuffer::AttachmentType::RenderBuffer, ITexture::Format::Depth32F }
	};
	constexpr static uint8_t kColorInd = 0;
};

class IDAdapter : RegisterUniformAdapter<IDAdapter>
{
public:
	inline static const Descriptor ourDescriptor{
		{ Descriptor::UniformType::Int }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};