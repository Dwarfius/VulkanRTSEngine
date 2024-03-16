#include "Precomp.h"
#include "IDRenderPass.h"

#include <Engine/VisualObject.h>
#include <Engine/Game.h>
#include <Engine/GameObject.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>
#include <Engine/Graphics/Adapters/TerrainAdapter.h>
#include <Engine/Graphics/Adapters/ObjectMatricesAdapter.h>
#include <Engine/Graphics/Adapters/SkeletonAdapter.h>
#include <Engine/Graphics/RenderPasses/GenericRenderPasses.h>

#include <Graphics/Camera.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/Resources/Texture.h>

namespace
{
	struct IDGOAdapterSourceData : UniformAdapterSource
	{
		IDRenderPass::ObjID myID;
	};

	struct IDTerrainAdapterSourceData : TerrainAdapter::Source
	{
		IDRenderPass::ObjID myID;
	};
}

IDRenderPass::IDRenderPass(Graphics& aGraphics, 
	const Handle<GPUPipeline>& aDefaultPipeline,
	const Handle<GPUPipeline>& aSkinningPipeline,
	const Handle<GPUPipeline>& aTerrainPipeline
)
	: myDefaultPipeline(aDefaultPipeline)
	, mySkinningPipeline(aSkinningPipeline)
	, myTerrainPipeline(aTerrainPipeline)
{
	aGraphics.AddNamedFrameBuffer(
		IDFrameBuffer::kName, 
		IDFrameBuffer::kDescriptor
	);

	PreallocateUBOs(IDAdapter::ourDescriptor.GetBlockSize());
	PreallocateUBOs(ObjectMatricesAdapter::ourDescriptor.GetBlockSize());
	PreallocateUBOs(SkeletonAdapter::ourDescriptor.GetBlockSize());
	PreallocateUBOs(TerrainAdapter::ourDescriptor.GetBlockSize());
}

void IDRenderPass::Execute(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("IDRenderPass::Execute");
	switch (myState)
	{
	case State::None:
		break;
	case State::Schedule:
		myFrameGOs.myGOCounter = 0;
		myFrameGOs.myTerrainCounter = 0;
		myState = State::Render;
		break;
	case State::Render:
		ASSERT(myDownloadTexture->GetPixels());
		ResolveClick();

		delete myDownloadTexture;
		myDownloadTexture = nullptr;
		myState = State::None;
		break;
	}

	RenderPass::Execute(aGraphics);

	if (myState == State::None) [[likely]]
	{
		return;
	}

	// TODO: get rid of singleton access here - pass from Game/Graphics
	Game& game = *Game::GetInstance();
	RenderPassJob& job = aGraphics.CreateRenderPassJob(CreateContext(aGraphics));
	ScheduleGameObjects(aGraphics, game, job);
	ScheduleTerrain(aGraphics, game, job);
}

void IDRenderPass::ScheduleGameObjects(Graphics& aGraphics, Game& aGame, RenderPassJob& aJob)
{
	if (myDefaultPipeline->GetState() != GPUResource::State::Valid
		|| mySkinningPipeline->GetState() != GPUResource::State::Valid)
		[[unlikely]]
	{
		return;
	}

	const Camera& camera = *aGame.GetCamera();
	aGame.ForEachRenderable([&](Renderable& aRenderable) {
		VisualObject& vo = aRenderable.myVO;

		if (!camera.CheckSphere(vo.GetCenter(), vo.GetRadius()))
		{
			return;
		}

		Handle<GPUModel>& model = vo.GetModel();
		if (!model.IsValid() || model->GetState() != GPUResource::State::Valid)
			[[unlikely]]
		{
			return;
		}

		// assuming we'll be able to render the GO
		// save it for tracking
		ObjID newID = myFrameGOs.myGOCounter++;
		if (newID >= kMaxObjects)
		{
			return;
		}
		myFrameGOs.myGOs[newID] = aRenderable.myGO;

		// updating the uniforms - grabbing game state!
		IDGOAdapterSourceData source{
			aGraphics,
			camera,
			aRenderable.myGO,
			vo,
			newID + 1
		};

		const bool isSkinned = aRenderable.myGO->GetSkeleton().IsValid();
		GPUPipeline* gpuPipeline = isSkinned ?
			mySkinningPipeline.Get() :
			myDefaultPipeline.Get();
		RenderJob::UniformSet uniformSet;
		if (!FillUBOs(uniformSet, aGraphics, source, *gpuPipeline))
			[[unlikely]]
		{
			return;
		}

		RenderJob& job = aJob.AllocateJob();
		job.SetPipeline(gpuPipeline);
		job.SetModel(model.Get());
		job.GetUniformSet() = uniformSet;

		RenderJob::IndexedDrawParams drawParams;
		drawParams.myOffset = 0;
		drawParams.myCount = model->GetPrimitiveCount();
		job.SetDrawParams(drawParams);
	});
}

void IDRenderPass::ScheduleTerrain(Graphics& aGraphics, Game& aGame, RenderPassJob& aJob)
{
	if (myTerrainPipeline->GetState() != GPUResource::State::Valid)
		[[unlikely]]
	{
		return;
	}

	const Camera& camera = *aGame.GetCamera();
	aGame.ForEachTerrain([&](const Game::TerrainEntity& anEntity) {
		VisualObject* visObj = anEntity.myVisualObject;
		if (!visObj)
		{
			return;
		}

		Handle<GPUTexture>& texture = visObj->GetTexture();
		if (!texture.IsValid() || texture->GetState() != GPUResource::State::Valid)
			[[unlikely]]
		{
			return;
		}

		// assuming we'll be able to render the terrain
		// save it for tracking
		ObjID newID = myFrameGOs.myTerrainCounter++;
		if (newID >= kMaxObjects)
		{
			return;
		}
		Terrain& terrain = *anEntity.myTerrain;
		myFrameGOs.myTerrains[newID] = &terrain;

		// updating the uniforms - grabbing game state!
		IDTerrainAdapterSourceData source{
			aGraphics,
			camera,
			nullptr,
			*visObj,
			terrain,
			newID | kTerrainBit
		};

		GPUPipeline* gpuPipeline = myTerrainPipeline.Get();
		RenderJob::UniformSet uniformSet;
		if (!FillUBOs(uniformSet, aGraphics, source, *gpuPipeline))
			[[unlikely]]
		{
			return;
		}

		RenderJob& job = aJob.AllocateJob();
		job.SetPipeline(gpuPipeline);
		job.GetTextures().PushBack(texture.Get());
		job.GetUniformSet() = uniformSet;

		RenderJob::TesselationDrawParams drawParams;
		const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
		drawParams.myInstanceCount = gridTiles.x * gridTiles.y;
		job.SetDrawParams(drawParams);
	});
}

void IDRenderPass::GetPickedEntity(glm::uvec2 aMousePos, Callback aCallback)
{
	ASSERT_STR(myState == State::None, "Only support 1 picking request at the same time!");
	myCallback = aCallback;
	myMousePos = aMousePos;
	myDownloadTexture = new Texture();
	myState = State::Schedule;
}

RenderContext IDRenderPass::CreateContext(Graphics& aGraphics) const
{
	return {
		.myFrameBuffer = IDFrameBuffer::kName,
		.myDownloadTexture =
			myState == State::Render ? myDownloadTexture : nullptr,
		.myTextureCount = 1u,
		.myTexturesToActivate = { 0 }, // for Terrain
		.myViewportSize = { 
			static_cast<int>(aGraphics.GetWidth()),
			static_cast<int>(aGraphics.GetHeight())
		},
		.myEnableDepthTest = true,
		.myEnableCulling = true,
		.myShouldClearColor = true,
		.myShouldClearDepth = true,
	};
}

void IDAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const UniformAdapterSource& genericData = static_cast<const UniformAdapterSource&>(aData);
	if (genericData.myGO) [[likely]]
	{
		const IDGOAdapterSourceData& source = 
			static_cast<const IDGOAdapterSourceData&>(aData);
		aUB.SetUniform(0, 0, source.myID);
	}
	else
	{
		const IDTerrainAdapterSourceData& source = 
			static_cast<const IDTerrainAdapterSourceData&>(aData);
		aUB.SetUniform(0, 0, source.myID);
	}
}

void IDRenderPass::ResolveClick()
{
	ASSERT_STR(myMousePos.x < myDownloadTexture->GetWidth(), 
		"Mouse outside of the viewport!");
	ASSERT_STR(myMousePos.y < myDownloadTexture->GetHeight(),
		"Mouse outside of the viewport!");

	const uint32_t y = myDownloadTexture->GetHeight() - myMousePos.y;
	const uint32_t pixelInd = y * myDownloadTexture->GetWidth() + myMousePos.x;
	const unsigned char* pixel = myDownloadTexture->GetPixels() + pixelInd * sizeof(ObjID);
	ObjID pickedID = *pixel;
	pickedID |= *(pixel + 1) << 8;
	pickedID |= *(pixel + 2) << 16;
	pickedID |= *(pixel + 3) << 24;

	PickedObject pickedObj;
	if (pickedID >= kTerrainBit)
	{
		pickedObj = myFrameGOs.myTerrains[pickedID - kTerrainBit];
	}
	else if(pickedID > 0)
	{
		pickedObj = myFrameGOs.myGOs[pickedID - 1];
	}
	myCallback(pickedObj);
	myState = State::None;
}