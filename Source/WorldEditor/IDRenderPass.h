#pragma once

#include <Graphics/FrameBuffer.h>
#include <Graphics/RenderPass.h>
#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>
#include <Core/RefCounted.h>
#include <Core/StaticString.h>
#include <Core/RWBuffer.h>

struct Renderable;
class GameObject;
class Terrain;
class VisualObject;

class IDRenderPass : public RenderPass
{
public:
	using ObjID = uint32_t;
	// TODO: rework to be dynamic
	static constexpr ObjID kMaxObjects = 1024;
	static constexpr ObjID kMaxTerrains = 8;

public:
	constexpr static uint32_t kId = Utils::CRC32("IDRenderPass");
	IDRenderPass(Graphics& aGraphics, 
		const Handle<GPUPipeline>& aDefaultPipeline,
		const Handle<GPUPipeline>& aSkinningPipeline,
		const Handle<GPUPipeline>& aTerrainPipeline
	);

	Id GetId() const final { return kId; }

	void BeginPass(Graphics& aGraphics) override;
	void ScheduleRenderable(Graphics& aGraphics, Renderable& aRenderable, Camera& aCamera);
	void ScheduleTerrain(Graphics& aGraphics, Terrain& aTerrain, VisualObject& aVisObject, Camera& aCamera);
	void Process(RenderJob& aJob, const IParams& aParams) const final {}

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	bool HasDynamicRenderContext() const final { return true; }

	constexpr static ObjID kTerrainBit = 1 << 31;

	struct FrameObjs
	{
		// TODO: unify this
		std::array<GameObject*, kMaxObjects> myGOs;
		std::array<Terrain*, kMaxTerrains> myTerrains;
		std::atomic<ObjID> myGOCounter = 0;
		std::atomic<ObjID> myTerrainCounter = 0;
	};
	RWBuffer<FrameObjs, 3> myFrameGOs;
	Handle<GPUPipeline> myDefaultPipeline;
	Handle<GPUPipeline> mySkinningPipeline;
	Handle<GPUPipeline> myTerrainPipeline;
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