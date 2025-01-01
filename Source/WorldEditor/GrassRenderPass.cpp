#include "Precomp.h"
#include "GrassRenderPass.h"

#include <Engine/Graphics/Adapters/CameraAdapter.h>
#include <Engine/Graphics/Adapters/TerrainAdapter.h>
#include <Engine/Graphics/NamedFrameBuffers.h>
#include <Engine/Graphics/RenderPasses/DebugRenderPass.h>
#include <Engine/Game.h>
#include <Engine/Terrain.h>

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUBuffer.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUTexture.h>
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

	myTerrainBuffer = aGraphics.CreateGPUBuffer(TerrainAdapter::ourDescriptor.GetBlockSize(), 
		GraphicsConfig::kMaxFramesScheduled + 1, true
	);

	myCamBuffer = aGraphics.CreateGPUBuffer(CameraAdapter::ourDescriptor.GetBlockSize(), 
		GraphicsConfig::kMaxFramesScheduled + 1, true
	);

	static constexpr uint32_t kIndirectCommandSize = 5 * sizeof(uint32_t);
	myIndirectBuffer = aGraphics.CreateGPUBuffer(kIndirectCommandSize, 1, false);

	myBox = aGraphics.GetOrCreate(aBox).Get<GPUModel>();

	aGraphics.AddRenderPassDependency(DebugRenderPass::kId, kId);
}

void GrassRenderPass::Execute(Graphics& aGraphics)
{
	const Game& game = *Game::GetInstance();
	if (!myHeightmap.IsValid())
	{
		if (game.GetTerrainCount() == 0)
		{
			return;
		}

		myHeightmap = aGraphics.GetOrCreate(game.GetTerrain(0)->GetTextureHandle()).Get<GPUTexture>();
	}

	if (myComputePipeline->GetState() != GPUResource::State::Valid
		|| myDrawPipeline->GetState() != GPUResource::State::Valid
		|| myCamBuffer->GetState() != GPUResource::State::Valid
		|| myBox->GetState() != GPUResource::State::Valid
		|| myPosBuffer->GetState() != GPUResource::State::Valid
		|| myIndirectBuffer->GetState() != GPUResource::State::Valid
		|| myHeightmap->GetState() != GPUResource::State::Valid
		|| myTerrainBuffer->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	const EngineSettings& settings = game.GetEngineSettings();
	RenderContext renderContext{
		.myFrameBuffer = DefaultFrameBuffer::kName,
		.myTextureCount = 1u,
		.myTexturesToActivate = { 0, -1 },
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

	{
		UniformBlock camBlock(*myCamBuffer.Get());
		CameraAdapterSourceData source
		{
			aGraphics,
			*game.GetCamera()
		};
		CameraAdapter::FillUniformBlock(source, camBlock);
	}

	RenderPassJob::SetBufferCmd& camCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	camCmd.myBuffer = myCamBuffer.Get();
	camCmd.mySlot = CameraAdapter::kBindpoint;
	camCmd.myType = RenderPassJob::GPUBufferType::Uniform;

	{
		UniformBlock terrainBlock(*myTerrainBuffer.Get());
		Game::TerrainEntity terrain;
		game.AccessTerrains([&](auto terrains) { terrain = terrains[0]; });
		TerrainAdapter::Source source
		{
			aGraphics,
			*game.GetCamera(),
			nullptr,
			*terrain.myVisualObject,
			*terrain.myTerrain
		};
		TerrainAdapter::FillUniformBlock(source, terrainBlock);
	}

	RenderPassJob::SetBufferCmd& terrainCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	terrainCmd.myBuffer = myTerrainBuffer.Get();
	terrainCmd.mySlot = 1;
	terrainCmd.myType = RenderPassJob::GPUBufferType::Uniform;

	RenderPassJob::SetBufferCmd& posCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	posCmd.myBuffer = myPosBuffer.Get();
	posCmd.mySlot = 6;
	posCmd.myType = RenderPassJob::GPUBufferType::ShaderStorage;

	RenderPassJob::SetBufferCmd& indirectBufferCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	indirectBufferCmd.myBuffer = myIndirectBuffer.Get();
	indirectBufferCmd.mySlot = 7;
	indirectBufferCmd.myType = RenderPassJob::GPUBufferType::ShaderStorage;

	RenderPassJob::BindImageTexture& heightmapCmd = cmdBuffer.Write<RenderPassJob::BindImageTexture>();
	heightmapCmd.mySlot = 0;
	heightmapCmd.myTexture = myHeightmap.Get();
	heightmapCmd.myAccessType = RenderPassJob::AccessType::Read;

	RenderPassJob::DispatchCompute& computeCmd = cmdBuffer.Write<RenderPassJob::DispatchCompute>();
	computeCmd.myGroupsX = 1;
	computeCmd.myGroupsY = 1;
	computeCmd.myGroupsZ = 1;

	// ^^ COMPUTE ^^ | vv DRAW vv

	RenderPassJob::MemBarrier& barrierCmd = cmdBuffer.Write<RenderPassJob::MemBarrier>();
	barrierCmd.myType = RenderPassJob::MemBarrierType::ShaderStorageBuffer
		| RenderPassJob::MemBarrierType::CommandBuffer;

	cmdBuffer.Write<RenderPassJob::SetPipelineCmd>().myPipeline = myDrawPipeline.Get();

	RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd>();
	modelCmd.myModel = myBox.Get();

	RenderPassJob::SetBufferCmd& indirectSourceCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	indirectSourceCmd.myBuffer = myIndirectBuffer.Get();
	indirectSourceCmd.myType = RenderPassJob::GPUBufferType::DrawIndirect;

	RenderPassJob::DrawIndexedInstancedIndirect& drawCmd = cmdBuffer.Write<RenderPassJob::DrawIndexedInstancedIndirect>();
	drawCmd.myOffset = 0;
}