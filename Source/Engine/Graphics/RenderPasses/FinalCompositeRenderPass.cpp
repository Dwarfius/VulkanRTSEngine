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
	myDependencies.push_back(DebugRenderPass::kId);

	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
}

void FinalCompositeRenderPass::SubmitJobs(Graphics& aGraphics)
{
	if (myPipeline->GetState() != GPUResource::State::Valid
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();

	RenderJob job(myPipeline, aGraphics.GetFullScreenQuad(), {});
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);

	passJob.Add(job);
}

void FinalCompositeRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myFrameBuffer = "";
	aContext.myFrameBufferReadTextures[0] = {
		DefaultFrameBuffer::kName,
		DefaultFrameBuffer::kColorInd,
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());
}