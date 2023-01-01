#include "Precomp.h"
#include "FinalCompositeRenderPass.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/Model.h>

#include "Graphics/NamedFrameBuffers.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"

FinalCompositeRenderPass::FinalCompositeRenderPass(
	Graphics& aGraphics, Handle<Pipeline> aPipeline
)
{
	AddDependency(DebugRenderPass::kId);

	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
}

void FinalCompositeRenderPass::SubmitJobs(Graphics& aGraphics)
{
	if (myPipeline->GetState() != GPUResource::State::Valid
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	PrepareContext(aGraphics);

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();

	RenderJob& job = passJob.AllocateJob();
	job.SetPipeline(myPipeline.Get());
	job.SetModel(aGraphics.GetFullScreenQuad().Get());
	
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);
}

void FinalCompositeRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = "";
	aContext.myFrameBufferReadTextures[0] = {
		DefaultFrameBuffer::kName,
		DefaultFrameBuffer::kColorInd,
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());
}