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

	game.ForEachRenderable([&](Renderable& aRenderable) {
		VisualObject& visObj = aRenderable.myVO;

		if (!camera.CheckSphere(visObj.GetCenter(), visObj.GetRadius()))
		{
			return;
		}

		// Default Pass handling
		if (!IsUsable(visObj)) [[unlikely]]
		{
			return;
		}

		const GameObject* gameObject = aRenderable.myGO;

		// Before building a render-job, we need to try to 
		// pre-allocate all the the UBOs - since if we can't, 
		// we need to early out without spawning a job
		GPUPipeline* gpuPipeline = visObj.GetPipeline().Get();
		
		// updating the uniforms - grabbing game state!
		UniformAdapterSource source{
			aGraphics,
			camera,
			gameObject,
			visObj
		};
		RenderJob::UniformSet uniformSet;
		if (!FillUBOs(uniformSet, aGraphics, source, *gpuPipeline)) 
			[[unlikely]]
		{
			return;
		}
		
		// Building a render job
		RenderJob& renderJob = passJob.AllocateJob();
		renderJob.SetModel(visObj.GetModel().Get());
		renderJob.SetPipeline(gpuPipeline);
		renderJob.GetTextures().PushBack(visObj.GetTexture().Get());
		renderJob.GetUniformSet() = uniformSet;

		RenderJob::IndexedDrawParams drawParams;
		drawParams.myOffset = 0;
		drawParams.myCount = renderJob.GetModel()->GetPrimitiveCount();
		renderJob.SetDrawParams(drawParams);
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
