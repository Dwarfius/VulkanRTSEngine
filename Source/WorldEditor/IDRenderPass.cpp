#include "Precomp.h"
#include "IDRenderPass.h"

#include <Engine/VisualObject.h>
#include <Engine/GameObject.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>

#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/UniformBuffer.h>

namespace
{
	struct IDAdapterSourceData : UniformAdapterSource
	{
		IDRenderPass::ObjID myID;
	};
}

IDRenderPass::IDRenderPass(Graphics& aGraphics, 
	const Handle<GPUPipeline>& aDefaultPipeline,
	const Handle<GPUPipeline>& aSkinningPipeline
)
	: myDefaultPipeline(aDefaultPipeline)
	, mySkinningPipeline(aSkinningPipeline)
{
	aGraphics.AddNamedFrameBuffer(IDFrameBuffer::kName, IDFrameBuffer::kDescriptor);
}

void IDRenderPass::BeginPass(Graphics& aGraphics)
{
	RenderPass::BeginPass(aGraphics);

	myFrameGOs.Advance();
	myFrameGOs.GetWrite().myCounter = 1;
}

void IDRenderPass::ScheduleRenderable(Graphics& aGraphics, Renderable& aRenderable, Camera& aCamera)
{
	if (myDefaultPipeline->GetState() != GPUResource::State::Valid
		|| mySkinningPipeline->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	VisualObject& vo = aRenderable.myVO;
	Handle<GPUModel>& model = vo.GetModel();
	if (!model.IsValid() || model->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	// assuming we'll be able to render the GO
	// save it for tracking
	FrameObjs& currFrame = myFrameGOs.GetWrite();
	ObjID newID = currFrame.myCounter++;
	if (newID >= kMaxObjects)
	{
		return;
	}
	currFrame.myGOs[newID] = aRenderable.myGO;

	// updating the uniforms - grabbing game state!
	IDAdapterSourceData source{
		aGraphics,
		aCamera,
		aRenderable.myGO,
		vo,
		newID
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

void IDRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = IDFrameBuffer::kName;

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());

	aContext.myEnableDepthTest = true;
	aContext.myEnableCulling = true;

	aContext.myShouldClearColor = true;
	aContext.myShouldClearDepth = true;
}

void IDAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const IDAdapterSourceData& source = static_cast<const IDAdapterSourceData&>(aData);
	aUB.SetUniform(0, 0, source.myID);
}