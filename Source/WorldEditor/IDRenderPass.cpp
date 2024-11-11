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
#include <Graphics/Resources/GPUBuffer.h>
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
	CmdBuffer& cmdBuffer = job.GetCmdBuffer();
	cmdBuffer.Clear();

	ScheduleGameObjects(aGraphics, game, cmdBuffer);
	ScheduleTerrain(aGraphics, game, cmdBuffer);
}

void IDRenderPass::ScheduleGameObjects(Graphics& aGraphics, Game& aGame, CmdBuffer& aCmdBuffer)
{
	if (myDefaultPipeline->GetState() != GPUResource::State::Valid
		|| mySkinningPipeline->GetState() != GPUResource::State::Valid)
		[[unlikely]]
	{
		return;
	}

	// Returns worst case size
	constexpr auto GetMaxBufferSize = [](size_t aCount)
	{
		// each command needs an extra byte to identify it
		return aCount * (sizeof(RenderPassJob::SetPipelineCmd) + 1 +
			sizeof(RenderPassJob::SetModelCmd) + 1 +
			sizeof(RenderPassJob::SetBufferCmd) * 4 + 4 +
			sizeof(RenderPassJob::DrawIndexedCmd) + 1);
	};

	std::vector<CmdBuffer> perPageBuffers;
	aGame.AccessRenderables([&](StableVector<Renderable>& aRenderables)
	{
		aRenderables.ForEachPage([&](StableVector<Renderable>::Page& aPage, size_t anIndex) 
		{
			const size_t maxSize = GetMaxBufferSize(aPage.GetCount());
			ASSERT_STR(maxSize <= std::numeric_limits<uint32_t>::max(), "Overflow bellow!");
			perPageBuffers.emplace_back(static_cast<uint32_t>(maxSize));
		});

		const Camera& camera = *aGame.GetCamera();
		aRenderables.ParallelForEachPage([&](StableVector<Renderable>::Page& aPage, size_t anIndex) 
		{
			CmdBuffer& cmdBuffer = perPageBuffers[anIndex];
			aPage.ForEach([&](Renderable& aRenderable)
			{
				VisualObject& vo = aRenderable.myVO;
				if (!camera.CheckSphere(vo.GetCenter(), vo.GetRadius()))
				{
					return;
				}

				if (!aRenderable.myVO.IsValidForRendering()) [[unlikely]]
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
				if (!BindUBOs(cmdBuffer, aGraphics, source, *gpuPipeline))
					[[unlikely]]
				{
					return;
				}

				RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd, false>();
				pipelineCmd.myPipeline = gpuPipeline;

				GPUModel* gpuModel = vo.GetModel().Get();
				RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd, false>();
				modelCmd.myModel = gpuModel;

				RenderPassJob::DrawIndexedCmd& drawCmd = cmdBuffer.Write<RenderPassJob::DrawIndexedCmd, false>();
				drawCmd.myOffset = 0;
				drawCmd.myCount = gpuModel->GetPrimitiveCount();
			});
		});
	});

	for (const CmdBuffer& pageBuffer : perPageBuffers)
	{
		aCmdBuffer.CopyFrom(pageBuffer);
	}
}

void IDRenderPass::ScheduleTerrain(Graphics& aGraphics, Game& aGame, CmdBuffer& aCmdBuffer)
{
	if (myTerrainPipeline->GetState() != GPUResource::State::Valid)
		[[unlikely]]
	{
		return;
	}

	const Camera& camera = *aGame.GetCamera();
	aGame.AccessTerrains([&](std::span<Game::TerrainEntity> aTerrains)
	{
		GPUPipeline* gpuPipeline = myTerrainPipeline.Get();

		RenderPassJob::SetPipelineCmd& pipelineCmd = aCmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
		pipelineCmd.myPipeline = gpuPipeline;

		for (Game::TerrainEntity& anEntity : aTerrains)
		{
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
			if (!BindUBOsAndGrow(aCmdBuffer, aGraphics, source, *gpuPipeline))
				[[unlikely]]
			{
				return;
			}

			RenderPassJob::SetTextureCmd& textureCmd = aCmdBuffer.Write<RenderPassJob::SetTextureCmd>();
			textureCmd.mySlot = 0;
			textureCmd.myTexture = texture.Get();

			RenderPassJob::DrawTesselatedCmd& drawCmd = aCmdBuffer.Write<RenderPassJob::DrawTesselatedCmd>();
			drawCmd.myOffset = 0;
			drawCmd.myCount = 1; // we'll create quads from points
			const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(terrain);
			drawCmd.myInstanceCount = gridTiles.x * gridTiles.y;
		}
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
		aUB.SetUniform(ourDescriptor.GetOffset(0, 0), source.myID);
	}
	else
	{
		const IDTerrainAdapterSourceData& source = 
			static_cast<const IDTerrainAdapterSourceData&>(aData);
		aUB.SetUniform(ourDescriptor.GetOffset(0, 0), source.myID);
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