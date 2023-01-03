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
#include "Terrain.h"
#include "Game.h"

void DefaultRenderPass::Execute(Graphics& aGraphics)
{
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

	game.ForEachRenderable([&](Renderable& aRenderable) {
		VisualObject& visObj = aRenderable.myVO;

		if (!camera.CheckSphere(visObj.GetCenter(), visObj.GetRadius()))
		{
			return;
		}

		// Default Pass handling
		if (!IsUsable(visObj))
		{
			return;
		}

		const GameObject* gameObject = aRenderable.myGO;

		// Before building a render-job, we need to try to 
		// pre-allocate all the the UBOs - since if we can't, 
		// we need to early out without spawning a job
		const GPUPipeline* gpuPipeline = visObj.GetPipeline().Get<const GPUPipeline>();
		const size_t uboCount = gpuPipeline->GetAdapterCount();
		ASSERT_STR(uboCount < 4,
			"Tried to push %llu UBOs into a render job that supports only 4!",
			uboCount);

		// updating the uniforms - grabbing game state!
		UniformAdapterSource source{
			aGraphics,
			camera,
			gameObject,
			visObj
		};
		RenderJob::UniformSet uniformSet;
		for (size_t i = 0; i < uboCount; i++)
		{
			const UniformAdapter& uniformAdapter = gpuPipeline->GetAdapter(i);
			UniformBuffer* uniformBuffer = AllocateUBO(
				aGraphics,
				uniformAdapter.GetDescriptor().GetBlockSize()
			);
			if (!uniformBuffer)
			{
				return;
			}

			UniformBlock uniformBlock(*uniformBuffer, uniformAdapter.GetDescriptor());
			uniformAdapter.Fill(source, uniformBlock);
			uniformSet.PushBack(uniformBuffer);
		}
		// Building a render job
		RenderJob& renderJob = AllocateJob();
		renderJob.SetModel(visObj.GetModel().Get());
		renderJob.SetPipeline(visObj.GetPipeline().Get());
		renderJob.GetTextures().PushBack(visObj.GetTexture().Get());
		renderJob.GetUniformSet() = uniformSet;

		RenderJob::IndexedDrawParams drawParams;
		drawParams.myOffset = 0;
		drawParams.myCount = renderJob.GetModel()->GetPrimitiveCount();
		renderJob.SetDrawParams(drawParams);
	});
}

void DefaultRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = DefaultFrameBuffer::kName;

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());

	aContext.myShouldClearColor = true;
	aContext.myShouldClearDepth = true;

	aContext.myTexturesToActivate[0] = 0;

	const EngineSettings& settings = Game::GetInstance()->GetEngineSettings();
	aContext.myPolygonMode = settings.myUseWireframe ?
		RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}

TerrainRenderPass::TerrainRenderPass()
{
	AddDependency(DefaultRenderPass::kId);
}

void TerrainRenderPass::Execute(Graphics& aGraphics)
{
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

	game.ForEachTerrain([&](Game::TerrainEntity& anEntity) {
		VisualObject* visObj = anEntity.myVisualObject;
		if (!visObj)
		{
			return;
		}

		if (!IsUsable(*visObj))
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

		const GPUPipeline* gpuPipeline = visObj->GetPipeline().Get<const GPUPipeline>();
		const size_t uboCount = gpuPipeline->GetAdapterCount();
		ASSERT_STR(uboCount < 4,
			"Tried to push %llu UBOs into a render job that supports only 4!",
			uboCount);

		// updating the uniforms - grabbing game state!
		TerrainAdapter::Source source{
			aGraphics,
			camera,
			nullptr,
			*visObj,
			terrain
		};

		RenderJob::UniformSet uniformSet;
		for (size_t i = 0; i < uboCount; i++)
		{
			const UniformAdapter& uniformAdapter = gpuPipeline->GetAdapter(i);
			UniformBuffer* uniformBuffer = AllocateUBO(
				aGraphics,
				uniformAdapter.GetDescriptor().GetBlockSize()
			);
			if (!uniformBuffer)
			{
				return;
			}

			UniformBlock uniformBlock(*uniformBuffer, uniformAdapter.GetDescriptor());
			uniformAdapter.Fill(source, uniformBlock);
			uniformSet.PushBack(uniformBuffer);
		}

		RenderJob& renderJob = AllocateJob();
		renderJob.SetModel(visObj->GetModel().Get());
		renderJob.SetPipeline(visObj->GetPipeline().Get());
		renderJob.GetTextures().PushBack(visObj->GetTexture().Get());
		renderJob.GetUniformSet() = uniformSet;

		RenderJob::TesselationDrawParams drawParams;
		const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
		drawParams.myInstanceCount = gridTiles.x * gridTiles.y;
		renderJob.SetDrawParams(drawParams);
	});
}

void TerrainRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = DefaultFrameBuffer::kName;

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());

	aContext.myTexturesToActivate[0] = 0;

	const EngineSettings& settings = Game::GetInstance()->GetEngineSettings();
	aContext.myPolygonMode = settings.myUseWireframe ?
		RenderContext::PolygonMode::Line : RenderContext::PolygonMode::Fill;

	aContext.myEnableCulling = true;
	aContext.myEnableDepthTest = true;
}
