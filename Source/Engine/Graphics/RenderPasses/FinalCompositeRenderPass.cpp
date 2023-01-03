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

void FinalCompositeRenderPass::Execute(Graphics& aGraphics)
{
	RenderPass::Execute(aGraphics);

	if (myPipeline->GetState() != GPUResource::State::Valid
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateContext(aGraphics));
	RenderJob& job = passJob.AllocateJob();
	job.SetPipeline(myPipeline.Get());
	job.SetModel(aGraphics.GetFullScreenQuad().Get());
	
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);
}

RenderContext FinalCompositeRenderPass::CreateContext(Graphics& aGraphics) const
{
	return {
		.myFrameBufferReadTextures = {
			DefaultFrameBuffer::kName,
			DefaultFrameBuffer::kColorInd,
			RenderContext::FrameBufferTexture::Type::Color
		},
		.myFrameBuffer = "",
		.myViewportSize = {
			static_cast<int>(aGraphics.GetWidth()),
			static_cast<int>(aGraphics.GetHeight())
		}
	};
}