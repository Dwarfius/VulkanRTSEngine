#include "Precomp.h"
#include "GrassRenderPass.h"

#include <Engine/Graphics/Adapters/CameraAdapter.h>
#include <Engine/Game.h>

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUBuffer.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/RenderContext.h>
#include <Graphics/RenderPassJob.h>

#include <Core/Resources/AssetTracker.h>

GrassRenderPass::GrassRenderPass(Graphics& aGraphics)
{
	AssetTracker& assetTracker = aGraphics.GetAssetTracker();
	Handle<Pipeline> pipeline = assetTracker.GetOrCreate<Pipeline>("Editor/GrassComputePipeline.ppl");
	myComputePipeline = aGraphics.GetOrCreate(pipeline).Get<GPUPipeline>();

	static constexpr uint32_t kSize = 32 * 32 * 3 * 4;
	myPosBuffer = aGraphics.CreateShaderStorageBuffer(kSize);

	myCamBuffer = aGraphics.CreateUniformBuffer(CameraAdapter::ourDescriptor.GetBlockSize());
}

void GrassRenderPass::Execute(Graphics& aGraphics)
{
	if (myComputePipeline->GetState() != GPUResource::State::Valid
		|| myPosBuffer->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderPassJob& job = aGraphics.CreateRenderPassJob({});
	CmdBuffer& cmdBuffer = job.GetCmdBuffer();
	cmdBuffer.Clear();

	RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
	pipelineCmd.myPipeline = myComputePipeline.Get();

	RenderPassJob::SetBufferCmd& posCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	posCmd.myBuffer = myPosBuffer.Get();
	posCmd.mySlot = 6;

	UniformBlock camBlock(*myCamBuffer.Get());
	CameraAdapterSourceData source
	{
		aGraphics,
		*Game::GetInstance()->GetCamera()
	};
	CameraAdapter::FillUniformBlock(source, camBlock);

	RenderPassJob::SetBufferCmd& camCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	camCmd.myBuffer = myCamBuffer.Get();
	camCmd.mySlot = CameraAdapter::kBindpoint;

	RenderPassJob::DispatchCompute& computeCmd = cmdBuffer.Write<RenderPassJob::DispatchCompute>();
	computeCmd.myGroupsX = 1;
	computeCmd.myGroupsY = 1;
	computeCmd.myGroupsZ = 1;

	RenderPassJob::MemBarrier& barrierCmd = cmdBuffer.Write<RenderPassJob::MemBarrier>();
	barrierCmd.myType = RenderPassJob::MemBarrierType::ShaderStorageBuffer;
}