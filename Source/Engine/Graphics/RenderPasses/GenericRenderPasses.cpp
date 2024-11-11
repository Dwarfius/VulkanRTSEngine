#include "Precomp.h"
#include "GenericRenderPasses.h"

#include <Graphics/Camera.h>
#include <Graphics/Graphics.h>
#include <Graphics/GPUResource.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/GPUBuffer.h>

#include "Graphics/RenderPasses/LightRenderPass.h"
#include "Graphics/Adapters/AdapterSourceData.h"
#include "Graphics/Adapters/TerrainAdapter.h"
#include "Graphics/NamedFrameBuffers.h"
#include "Light.h"
#include "Terrain.h"
#include "Game.h"

DefaultRenderPass::DefaultRenderPass()
{
	AddDependency(LightRenderPass::kId);
}

void DefaultRenderPass::Execute(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("DefaultRenderPass::Execute");
	RenderPass::Execute(aGraphics);

	// TODO: get rid of singleton access here - pass from Game/Graphics
	Game& game = *Game::GetInstance();
	const Camera& camera = *game.GetCamera();

	constexpr auto IsUsable = [](const VisualObject& aVO) 
	{
		constexpr auto CheckResource = [](const Handle<GPUResource>& aRes) 
		{
			return aRes.IsValid() && aRes->GetState() == GPUResource::State::Valid;
		};
		return CheckResource(aVO.GetModel())
			&& CheckResource(aVO.GetPipeline())
			&& CheckResource(aVO.GetTexture());
	};

	// Returns worst case size
	constexpr auto GetMaxBufferSize = [](size_t aCount)
	{
		// each command needs an extra byte to identify it
		return aCount * (sizeof(RenderPassJob::SetPipelineCmd) + 1 +
			sizeof(RenderPassJob::SetModelCmd) + 1 +
			sizeof(RenderPassJob::SetTextureCmd) + 1 +
			sizeof(RenderPassJob::SetBufferCmd) * 4 + 4 +
			sizeof(RenderPassJob::DrawIndexedCmd) + 1);
	};

	std::vector<CmdBuffer> perPageBuffers;
	game.AccessRenderables([&](StableVector<Renderable>& aRenderables) 
	{
		perPageBuffers.resize(aRenderables.GetPageCount());
		aRenderables.ForEachPage([&](const StableVector<Renderable>::Page& aPage, size_t aPageInd)
		{
			const size_t maxSize = GetMaxBufferSize(aPage.GetCount());
			ASSERT_STR(maxSize <= std::numeric_limits<uint32_t>::max(), "Overflow bellow!");
			perPageBuffers[aPageInd].Resize(static_cast<uint32_t>(maxSize));
		});

		aRenderables.ParallelForEachPage([&](StableVector<Renderable>::Page& aPage, size_t aPageInd)
		{
			CmdBuffer& cmdBuffer = perPageBuffers[aPageInd];
			aPage.ForEach([&](Renderable& aRenderable)
			{
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

				{
					Profiler::ScopedMark earlyChecks("BindUBOs");
					// updating the uniforms - grabbing game state!
					UniformAdapterSource source{
						aGraphics,
						camera,
						gameObject,
						visObj
					};
					if (!BindUBOs(cmdBuffer, aGraphics, source, *gpuPipeline))
						[[unlikely]]
					{
						return;
					}
				}

				{
					Profiler::ScopedMark earlyChecks("BuildRenderJob");
					// Building a render job
					RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd, false>();
					pipelineCmd.myPipeline = gpuPipeline;

					RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd, false>();
					modelCmd.myModel = visObj.GetModel().Get();

					RenderPassJob::SetTextureCmd& textureCmd = cmdBuffer.Write<RenderPassJob::SetTextureCmd, false>();
					textureCmd.mySlot = 0;
					textureCmd.myTexture = visObj.GetTexture().Get();

					RenderPassJob::DrawIndexedCmd& drawCmd = cmdBuffer.Write<RenderPassJob::DrawIndexedCmd, false>();
					drawCmd.myOffset = 0;
					drawCmd.myCount = visObj.GetModel().Get()->GetPrimitiveCount();
				}
			});
		});
	});

	Profiler::ScopedMark earlyChecks("AgreggateCmds");
	// aggregate all 
	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateContext(aGraphics));
	CmdBuffer& passCmdBuffer = passJob.GetCmdBuffer();
	passCmdBuffer.Clear();
	for (const CmdBuffer& cmdBuffer : perPageBuffers)
	{
		passCmdBuffer.CopyFrom(cmdBuffer);
	}
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
	CmdBuffer& cmdBuffer = passJob.GetCmdBuffer();
	cmdBuffer.Clear();
	
	game.AccessTerrains([&](std::span<Game::TerrainEntity> aTerrains)
	{
		if (!aTerrains.empty())
		{
			RenderPassJob::SetTesselationPatchCPs& patchCmd = cmdBuffer.Write<RenderPassJob::SetTesselationPatchCPs>();
			patchCmd.myControlPointCount = 1;
		}

		for (Game::TerrainEntity& anEntity : aTerrains)
		{
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
			if (!BindUBOsAndGrow(cmdBuffer, aGraphics, source, *gpuPipeline))
				[[unlikely]]
			{
				return;
			}

			RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
			pipelineCmd.myPipeline = gpuPipeline;

			RenderPassJob::SetTextureCmd& textureCmd = cmdBuffer.Write<RenderPassJob::SetTextureCmd>();
			textureCmd.mySlot = 0;
			textureCmd.myTexture = visObj->GetTexture().Get();

			RenderPassJob::DrawTesselatedCmd& drawCmd = cmdBuffer.Write<RenderPassJob::DrawTesselatedCmd>();
			drawCmd.myOffset = 0;
			drawCmd.myCount = 1; // we'll create quads from points
			const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
			drawCmd.myInstanceCount = gridTiles.x * gridTiles.y;
		}
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
