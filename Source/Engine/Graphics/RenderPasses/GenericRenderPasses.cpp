#include "Precomp.h"
#include "GenericRenderPasses.h"

#include <Graphics/Camera.h>
#include <Graphics/Graphics.h>
#include <Graphics/GPUResource.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/UniformBuffer.h>

#include "Graphics/Adapters/AdapterSourceData.h"
#include "Graphics/Adapters/TerrainAdapter.h"
#include "Graphics/NamedFrameBuffers.h"
#include "Light.h"
#include "Terrain.h"
#include "Game.h"

void DefaultRenderPass::Execute(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("DefaultRenderPass::Execute");
	RenderPass::Execute(aGraphics);

	// TODO: get rid of singleton access here - pass from Game/Graphics
	Game& game = *Game::GetInstance();
	const Camera& camera = *game.GetCamera();

	constexpr auto IsUsable = [](const VisualObject& aVO) {
		constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) {
			return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
		};
		return CheckResource(aVO.GetModel())
			&& CheckResource(aVO.GetPipeline())
			&& CheckResource(aVO.GetTexture());
	};

	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateContext(aGraphics));
	CmdBuffer& cmdBuffer = passJob.GetCmdBuffer();
	const uint32_t maxSize = game.GetRenderableCount() *
		(sizeof(RenderPassJob::SetPipelineCmd) + 1 +
			sizeof(RenderPassJob::SetModelCmd) + 1 +
			sizeof(RenderPassJob::SetTextureCmd) + 1 +
			sizeof(RenderPassJob::SetUniformBufferCmd) * 4 + 4 +
			sizeof(RenderPassJob::DrawIndexedCmd) + 1);
	cmdBuffer.Resize(maxSize); // worst case
	cmdBuffer.Clear();
	tbb::spin_mutex cmdLock;

	game.ForEachRenderable([&](Renderable& aRenderable) {
		VisualObject& visObj = aRenderable.myVO;

		{
			Profiler::ScopedMark earlyChecks("EarlyChecks");
			if (!camera.CheckSphere(visObj.GetCenter(), visObj.GetRadius()))
			{
				return;
			}

			// Default Pass handling
			if (!visObj.IsValidForRendering()) [[unlikely]]
			{
				visObj.SetIsValidForRendering(IsUsable(visObj));
				return;
			}
		}

		const GameObject* gameObject = aRenderable.myGO;

		// Before building a render-job, we need to try to 
		// pre-allocate all the the UBOs - since if we can't, 
		// we need to early out without spawning a job
		GPUPipeline* gpuPipeline = visObj.GetPipeline().Get();
		
		RenderJob::UniformSet uniformSet;
		{
			Profiler::ScopedMark earlyChecks("FillUBO");
			// updating the uniforms - grabbing game state!
			UniformAdapterSource source{
				aGraphics,
				camera,
				gameObject,
				visObj
			};
			if (!FillUBOs(uniformSet, aGraphics, source, *gpuPipeline))
				[[unlikely]]
			{
				return;
			}
		}
		
		{
			Profiler::ScopedMark earlyChecks("BuildRenderJob");
			// Building a render job
			RenderPassJob::SetPipelineCmd* pipelineCmd;
			RenderPassJob::SetModelCmd* modelCmd;
			RenderPassJob::SetTextureCmd* textureCmd;
			RenderPassJob::SetUniformBufferCmd* uboCmd[4];
			RenderPassJob::DrawIndexedCmd* drawCmd;

			{
				tbb::spin_mutex::scoped_lock lock(cmdLock);
				pipelineCmd = &cmdBuffer.Write<RenderPassJob::SetPipelineCmd, false>();
				modelCmd = &cmdBuffer.Write<RenderPassJob::SetModelCmd, false>();
				textureCmd = &cmdBuffer.Write<RenderPassJob::SetTextureCmd, false>();
				for (uint8_t i = 0; i < uniformSet.GetSize(); i++)
				{
					uboCmd[i] = &cmdBuffer.Write<RenderPassJob::SetUniformBufferCmd, false>();
				}
				drawCmd = &cmdBuffer.Write<RenderPassJob::DrawIndexedCmd, false>();
			}

			pipelineCmd->myPipeline = gpuPipeline;
			modelCmd->myModel = visObj.GetModel().Get();
			textureCmd->mySlot = 0;
			textureCmd->myTexture = visObj.GetTexture().Get();
			for (uint8_t i = 0; i < uniformSet.GetSize(); i++)
			{
				uboCmd[i]->mySlot = i;
				uboCmd[i]->myUniformBuffer = uniformSet[i];
			}
			drawCmd->myOffset = 0;
			drawCmd->myCount = visObj.GetModel().Get()->GetPrimitiveCount();
		}
	});
}

RenderContext DefaultRenderPass::CreateContext(Graphics& aGraphics) const
{
	const EngineSettings& settings = Game::GetInstance()->GetEngineSettings();
	return {
		.myFrameBuffer = DefaultFrameBuffer::kName,
		.myTextureCount = 1u,
		.myTexturesToActivate = { 0 },
		.myViewportSize = {
			static_cast<int>(aGraphics.GetWidth()),
			static_cast<int>(aGraphics.GetHeight())
		},
		.myPolygonMode = settings.myUseWireframe ?
			RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill,
		.myEnableDepthTest = true,
		.myEnableCulling = true,
		.myShouldClearColor = true,
		.myShouldClearDepth = true,
	};
}

TerrainRenderPass::TerrainRenderPass()
{
	AddDependency(DefaultRenderPass::kId);
}

void TerrainRenderPass::Execute(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("DefaultRenderPass::Execute");
	RenderPass::Execute(aGraphics);

	// TODO: get rid of singleton access here - pass from Game/Graphics
	Game& game = *Game::GetInstance();
	const Camera& camera = *game.GetCamera();

	constexpr auto IsUsable = [](const VisualObject& aVO) {
		constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) {
			return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
		};
		return CheckResource(aVO.GetPipeline())
			&& CheckResource(aVO.GetTexture());
	};

	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateContext(aGraphics));
	game.ForEachTerrain([&](Game::TerrainEntity& anEntity) {
		VisualObject* visObj = anEntity.myVisualObject;
		if (!visObj)
			[[unlikely]]
		{
			return;
		}

		if (!IsUsable(*visObj))
			[[unlikely]]
		{
			return;
		}

		if (visObj->GetModel().IsValid()
			&& !camera.CheckSphere(visObj->GetTransform().GetPos(), visObj->GetRadius()))
		{
			return;
		}

		// building a render job
		const Terrain& terrain = *anEntity.myTerrain;

		GPUPipeline* gpuPipeline = visObj->GetPipeline().Get();

		// updating the uniforms - grabbing game state!
		TerrainAdapter::Source source{
			aGraphics,
			camera,
			nullptr,
			*visObj,
			terrain
		};
		RenderJob::UniformSet uniformSet;
		if (!FillUBOs(uniformSet, aGraphics, source, *gpuPipeline))
			[[unlikely]]
		{
			return;
		}

		RenderJob& renderJob = passJob.AllocateJob();
		renderJob.SetModel(visObj->GetModel().Get());
		renderJob.SetPipeline(gpuPipeline);
		renderJob.GetTextures().PushBack(visObj->GetTexture().Get());
		renderJob.GetUniformSet() = uniformSet;

		RenderJob::TesselationDrawParams drawParams;
		const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
		drawParams.myInstanceCount = gridTiles.x * gridTiles.y;
		renderJob.SetDrawParams(drawParams);
	});
}

RenderContext TerrainRenderPass::CreateContext(Graphics& aGraphics) const
{
	const EngineSettings& settings = Game::GetInstance()->GetEngineSettings();
	return {
		.myFrameBuffer = DefaultFrameBuffer::kName,
		.myTextureCount = 1u,
		.myTexturesToActivate = { 0 },
		.myViewportSize = {
			static_cast<int>(aGraphics.GetWidth()),
			static_cast<int>(aGraphics.GetHeight())
		},
		.myPolygonMode = settings.myUseWireframe ?
			RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill,
		.myEnableDepthTest = true,
		.myEnableCulling = true,
	};
}
