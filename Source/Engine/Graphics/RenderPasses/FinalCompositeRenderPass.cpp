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
	Profiler::ScopedMark mark("FinalCompositeRenderPass::Execute");
	RenderPass::Execute(aGraphics);

	if (myPipeline->GetState() != GPUResource::State::Valid
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateContext(aGraphics));
	CmdBuffer& cmdBuffer = passJob.GetCmdBuffer();
	// it can be reused, but it's such a small job anyway
	cmdBuffer.Clear();

	RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
	pipelineCmd.myPipeline = myPipeline.Get();

	RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd>();
	modelCmd.myModel = aGraphics.GetFullScreenQuad().Get();

	RenderPassJob::DrawArrayCmd& drawCmd = cmdBuffer.Write<RenderPassJob::DrawArrayCmd>();
	drawCmd.myDrawMode = IModel::PrimitiveType::Triangles;
	drawCmd.myOffset = 0;
	drawCmd.myCount = 6;
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