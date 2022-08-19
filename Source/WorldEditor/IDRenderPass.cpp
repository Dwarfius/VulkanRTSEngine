#include "Precomp.h"
#include "IDRenderPass.h"

#include <Engine/VisualObject.h>
#include <Engine/GameObject.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>
#include <Engine/Graphics/Adapters/TerrainAdapter.h>
#include <Engine/Graphics/RenderPasses/GenericRenderPasses.h>

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
	aGraphics.AddNamedFrameBuffer(IDFrameBuffer::kName, IDFrameBuffer::kDescriptor);
}

void IDRenderPass::BeginPass(Graphics& aGraphics)
{
	RenderPass::BeginPass(aGraphics);

	if (myState == State::None)
	{
		return;
	}

	if(myState == State::Render)
	{
		ASSERT(myDownloadTexture->GetPixels());
		ResolveClick();

		delete myDownloadTexture;
		myDownloadTexture = nullptr;
		myState = State::None;
		return;
	}

	myFrameGOs.myGOCounter = 0;
	myFrameGOs.myTerrainCounter = 0;
	myState = State::Render;
}

void IDRenderPass::ScheduleRenderable(Graphics& aGraphics, Renderable& aRenderable, Camera& aCamera)
{
	if (myState == State::None) [[likely]]
	{
		return;
	}

	if (myDefaultPipeline->GetState() != GPUResource::State::Valid
		|| mySkinningPipeline->GetState() != GPUResource::State::Valid)
		[[unlikely]]
	{
		return;
	}

	VisualObject& vo = aRenderable.myVO;
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
		aCamera,
		aRenderable.myGO,
		vo,
		newID + 1
	};

	const bool isSkinned = aRenderable.myGO->GetSkeleton().IsValid();
	const GPUPipeline* gpuPipeline = isSkinned ?
		mySkinningPipeline.Get<const GPUPipeline>() :
		myDefaultPipeline.Get<const GPUPipeline>();
	const size_t uboCount = gpuPipeline->GetAdapterCount();
	RenderJob::UniformSet uniformSet;
	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = gpuPipeline->GetAdapter(i);
		UniformBuffer* ubo = AllocateUBO(
			aGraphics, 
			uniformAdapter.GetDescriptor().GetBlockSize()
		);
		if (!ubo)
		{
			return;
		}

		UniformBlock uniformBlock(*ubo, uniformAdapter.GetDescriptor());
		uniformAdapter.Fill(source, uniformBlock);
		uniformSet.PushBack(ubo);
	}

	RenderJob& job = AllocateJob();
	job.SetPipeline(isSkinned ? mySkinningPipeline.Get() : myDefaultPipeline.Get());
	job.SetModel(model.Get());
	job.GetUniformSet() = uniformSet;

	RenderJob::IndexedDrawParams drawParams;
	drawParams.myOffset = 0;
	drawParams.myCount = model->GetPrimitiveCount();
	job.SetDrawParams(drawParams);
}

void IDRenderPass::ScheduleTerrain(Graphics& aGraphics, Terrain& aTerrain, VisualObject& aVisObject, Camera& aCamera)
{
	if (myState == State::None) [[likely]]
	{
		return;
	}

	if (myTerrainPipeline->GetState() != GPUResource::State::Valid)
		[[unlikely]]
	{
		return;
	}

	Handle<GPUTexture>& texture = aVisObject.GetTexture();
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
	myFrameGOs.myTerrains[newID] = &aTerrain;

	// updating the uniforms - grabbing game state!
	IDTerrainAdapterSourceData source{
		aGraphics,
		aCamera,
		nullptr,
		aVisObject,
		aTerrain,
		newID | kTerrainBit
	};

	const GPUPipeline* gpuPipeline = myTerrainPipeline.Get<const GPUPipeline>();
	const size_t uboCount = gpuPipeline->GetAdapterCount();
	RenderJob::UniformSet uniformSet;
	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = gpuPipeline->GetAdapter(i);
		UniformBuffer* ubo = AllocateUBO(
			aGraphics,
			uniformAdapter.GetDescriptor().GetBlockSize()
		);
		if (!ubo)
		{
			return;
		}

		UniformBlock uniformBlock(*ubo, uniformAdapter.GetDescriptor());
		uniformAdapter.Fill(source, uniformBlock);
		uniformSet.PushBack(ubo);
	}

	RenderJob& job = AllocateJob();
	job.SetPipeline(myTerrainPipeline.Get());
	job.GetTextures().PushBack(texture.Get());
	job.GetUniformSet() = uniformSet;

	RenderJob::TesselationDrawParams drawParams;
	const glm::ivec2 gridTiles = TerrainAdapter::GetTileCount(aTerrain);
	drawParams.myInstanceCount = gridTiles.x * gridTiles.y;
	job.SetDrawParams(drawParams);
}

void IDRenderPass::GetPickedEntity(glm::uvec2 aMousePos, Callback aCallback)
{
	ASSERT_STR(myState == State::None, "Only support 1 picking request at the same time!");
	myCallback = aCallback;
	myMousePos = aMousePos;
	myDownloadTexture = new Texture();
	myState = State::Schedule;
}

void IDRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = IDFrameBuffer::kName;

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());

	aContext.myEnableDepthTest = true;
	aContext.myEnableCulling = true;

	aContext.myShouldClearColor = true;
	aContext.myShouldClearDepth = true;

	aContext.myTexturesToActivate[0] = 0; // for Terrain

	aContext.myDownloadTexture = myState == State::Schedule ? 
		myDownloadTexture : nullptr;
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