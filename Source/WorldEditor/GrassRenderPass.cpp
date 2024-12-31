#include "Precomp.h"
#include "GrassRenderPass.h"

#include <Engine/Graphics/Adapters/CameraAdapter.h>
#include <Engine/Graphics/NamedFrameBuffers.h>
#include <Engine/Graphics/RenderPasses/DebugRenderPass.h>
#include <Engine/Game.h>

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUBuffer.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/RenderContext.h>
#include <Graphics/RenderPassJob.h>

#include <Core/Resources/AssetTracker.h>

GrassRenderPass::GrassRenderPass(Graphics& aGraphics, const Handle<Model>& aBox)
{
	AssetTracker& assetTracker = aGraphics.GetAssetTracker();
	Handle<Pipeline> computePipeline = assetTracker.GetOrCreate<Pipeline>("Editor/GrassComputePipeline.ppl");
	myComputePipeline = aGraphics.GetOrCreate(computePipeline).Get<GPUPipeline>();

	Handle<Pipeline> drawPipeline = assetTracker.GetOrCreate<Pipeline>("Editor/GrassInstancedPipeline.ppl");
	myDrawPipeline = aGraphics.GetOrCreate(drawPipeline).Get<GPUPipeline>();

	static constexpr uint32_t kSize = 32 * 32 * 3 * 4;
	myPosBuffer = aGraphics.CreateGPUBuffer(kSize, 1, false);

	myCamBuffer = aGraphics.CreateGPUBuffer(CameraAdapter::ourDescriptor.GetBlockSize(), 
		GraphicsConfig::kMaxFramesScheduled + 1, true
	);

	myBox = aGraphics.GetOrCreate(aBox).Get<GPUModel>();

	aGraphics.AddRenderPassDependency(DebugRenderPass::kId, kId);
}

void GrassRenderPass::Execute(Graphics& aGraphics)
{
	if (myComputePipeline->GetState() != GPUResource::State::Valid
		|| myDrawPipeline->GetState() != GPUResource::State::Valid
		|| myCamBuffer->GetState() != GPUResource::State::Valid
		|| myBox->GetState() != GPUResource::State::Valid
		|| myPosBuffer->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	const EngineSettings& settings = Game::GetInstance()->GetEngineSettings();
	RenderContext renderContext{
		.myFrameBuffer = DefaultFrameBuffer::kName,
		.myTextureCount = 1u,
		.myTexturesToActivate = { -1 },
		.myViewportSize = {
			static_cast<int>(aGraphics.GetWidth()),
			static_cast<int>(aGraphics.GetHeight())
		},
		.myPolygonMode = settings.myUseWireframe ?
			RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill,
		.myEnableDepthTest = true,
		.myEnableCulling = true,
	};
	RenderPassJob& job = aGraphics.CreateRenderPassJob(renderContext);
	CmdBuffer& cmdBuffer = job.GetCmdBuffer();
	cmdBuffer.Clear();

	RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
	pipelineCmd.myPipeline = myComputePipeline.Get();

	RenderPassJob::SetBufferCmd& posCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	posCmd.myBuffer = myPosBuffer.Get();
	posCmd.mySlot = 6;
	posCmd.myType = RenderPassJob::GPUBufferType::ShaderStorage;

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
	camCmd.myType = RenderPassJob::GPUBufferType::Uniform;

	RenderPassJob::DispatchCompute& computeCmd = cmdBuffer.Write<RenderPassJob::DispatchCompute>();
	computeCmd.myGroupsX = 1;
	computeCmd.myGroupsY = 1;
	computeCmd.myGroupsZ = 1;

	RenderPassJob::MemBarrier& barrierCmd = cmdBuffer.Write<RenderPassJob::MemBarrier>();
	barrierCmd.myType = RenderPassJob::MemBarrierType::ShaderStorageBuffer;

	cmdBuffer.Write<RenderPassJob::SetPipelineCmd>().myPipeline = myDrawPipeline.Get();

	RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd>();
	modelCmd.myModel = myBox.Get();

	RenderPassJob::DrawIndexedInstanced& drawCmd = cmdBuffer.Write<RenderPassJob::DrawIndexedInstanced>();
	drawCmd.myCount = 36;
	drawCmd.myOffset = 0;
	drawCmd.myInstanceCount = 32 * 32;
}