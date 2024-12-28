#pragma once

#include "RenderContext.h"

#include <Core/CmdBuffer.h>

class GPUPipeline;
class GPUTexture;
class GPUModel;
class GPUBuffer;
class Graphics;

// A basic class encapsulating a set of render commands
// with a specific context for the next frame. Needs to be specialized for
// different rendering backends!
class RenderPassJob
{
public:
	template<uint8_t Id>
	struct RenderPassJobCmd
	{
		static constexpr uint8_t kId = Id;
	};

	struct SetModelCmd : RenderPassJobCmd<0>
	{
		GPUModel* myModel;
	};

	struct SetPipelineCmd : RenderPassJobCmd<1>
	{
		GPUPipeline* myPipeline;
	};

	struct SetTextureCmd : RenderPassJobCmd<2>
	{
		uint8_t mySlot;
		GPUTexture* myTexture;
	};

	struct SetBufferCmd : RenderPassJobCmd<3>
	{
		uint8_t mySlot;
		GPUBuffer* myBuffer;
	};

	struct DrawIndexedCmd : RenderPassJobCmd<4>
	{
		uint32_t myOffset;
		uint32_t myCount;
	};

	// TODO: remove scissor rect from context
	struct SetScissorRectCmd : RenderPassJobCmd<5>
	{
		// X, Y, Width, Height
		int32_t myRect[4];
	};

	struct DrawTesselatedCmd : RenderPassJobCmd<6>
	{
		uint32_t myOffset;
		uint32_t myCount;
		uint32_t myInstanceCount;
	};

	struct SetTesselationPatchCPs : RenderPassJobCmd<7>
	{
		uint32_t myControlPointCount;
	};

	struct DrawArrayCmd : RenderPassJobCmd<8>
	{
		uint32_t myOffset;
		uint32_t myCount;
		uint8_t myDrawMode;
	};

	struct DispatchCompute : RenderPassJobCmd<9>
	{
		uint32_t myGroupsX;
		uint32_t myGroupsY;
		uint32_t myGroupsZ;
	};

	enum class MemBarrierType : uint8_t
	{
		UniformBuffer = 0b1,
		ShaderStorageBuffer = 0b10,
		CommandBuffer = 0b100
	};
	// TODO: isolate Windows.h, can't use MemoryBarrier!
	struct MemBarrier : RenderPassJobCmd<10>
	{
		MemBarrierType myType;
	};

	struct DrawIndexedInstanced : RenderPassJobCmd<11>
	{
		uint32_t myOffset;
		uint32_t myCount;
		uint32_t myInstanceCount;
	};

public:
	virtual ~RenderPassJob() = default;

	// Not thread safe
	CmdBuffer& GetCmdBuffer() { return myCmdBuffer; }

	void Execute(Graphics& aGraphics);

	void Initialize(const RenderContext& aContext);

protected:
	const RenderContext& GetRenderContext() const { return myContext; }

private:
	// called immediatelly after creating a job
	virtual void OnInitialize(const RenderContext& aContext) = 0;
	// called first before running jobs
	virtual void BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext) = 0;
	// always called before running jobs
	virtual void Clear(const RenderContext& aContext) = 0;
	// called just before executing the jobs
	virtual void SetupContext(Graphics& aGraphics, const RenderContext& aContext) = 0;
	virtual void RunCommands(const CmdBuffer& aCmdBuffer) = 0;
	// called if the user has requested the result of rendering to the framebuffer
	// to be downloaded back to CPU
	virtual void DownloadFrameBuffer(Graphics& aGraphics, Texture& aTexture) = 0;

	RenderContext myContext;
	CmdBuffer myCmdBuffer;
};

inline RenderPassJob::MemBarrierType operator&(
	RenderPassJob::MemBarrierType aLeft, RenderPassJob::MemBarrierType aRight
)
{
	return static_cast<RenderPassJob::MemBarrierType>(
		std::to_underlying(aLeft) & std::to_underlying(aRight)
	);
}