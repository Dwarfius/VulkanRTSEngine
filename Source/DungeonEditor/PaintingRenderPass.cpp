#include "Precomp.h"
#include "PaintingRenderPass.h"

#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUIRendering.h>
#include <Engine/Graphics/Adapters/ObjectMatricesAdapter.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Camera.h>

void PaintingRenderPass::SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics)
{
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	aPipeline->ExecLambdaOnLoad([this](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		myBlock = std::make_shared<UniformBlock>(pipeline->GetDescriptor(0));
	});
}

std::string_view PaintingRenderPass::GetWriteBuffer() const
{
	return myWriteToOther ?
		static_cast<std::string_view>(OtherPaintingFrameBuffer::kName) :
		static_cast<std::string_view>(PaintingFrameBuffer::kName);
}

void PaintingRenderPass::SubmitJobs(Graphics& aGraphics)
{
	if (!myPipeline.IsValid() ||
		myPipeline->GetState() != GPUResource::State::Valid ||
		aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderJob job(myPipeline, aGraphics.GetFullScreenQuad(), {});
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);

	PainterAdapter::Source source{
		aGraphics,
		myParams.myCamera,
		myParams.myColor,
		myParams.myTexSize,
		myParams.myPrevMousePos,
		myParams.myMousePos,
		myParams.myGridDims,
		myParams.myPaintMode,
		myParams.myBrushSize
	};
	PainterAdapter adapter;
	adapter.FillUniformBlock(source, *myBlock);
	job.AddUniformBlock(*myBlock);

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();
	passJob.Add(job);

	myWriteToOther = !myWriteToOther;
}

void PaintingRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = GetWriteBuffer();
	aContext.myFrameBufferDrawSlots[0] = PaintingFrameBuffer::kFinalColor;
	aContext.myFrameBufferDrawSlots[1] = PaintingFrameBuffer::kPaintingColor;
	aGraphics.GetRenderPass<DisplayRenderPass>()->SetReadBuffer(GetWriteBuffer());

	aContext.myFrameBufferReadTextures[0] = {
		myWriteToOther ?
			static_cast<std::string_view>(PaintingFrameBuffer::kName) :
			static_cast<std::string_view>(OtherPaintingFrameBuffer::kName),
		PaintingFrameBuffer::kPaintingColor,
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());
	aContext.myEnableDepthTest = false;
}

void DisplayRenderPass::SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics)
{
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	aPipeline->ExecLambdaOnLoad([this](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		myBlock = std::make_shared<UniformBlock>(pipeline->GetDescriptor(0));
	});
}

void DisplayRenderPass::SubmitJobs(Graphics& aGraphics)
{
	if (!myPipeline.IsValid() ||
		myPipeline->GetState() != GPUResource::State::Valid ||
		aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderJob job(myPipeline, aGraphics.GetFullScreenQuad(), {});
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);

	PainterAdapter::Source source{
		aGraphics,
		myParams.myCamera,
		myParams.myColor,
		myParams.myTexSize,
		myParams.myPrevMousePos,
		myParams.myMousePos,
		myParams.myGridDims,
		myParams.myPaintMode,
		myParams.myBrushSize
	};
	PainterAdapter adapter;
	adapter.FillUniformBlock(source, *myBlock);
	job.AddUniformBlock(*myBlock);

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();
	passJob.Add(job);
}

void DisplayRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = "";
	aGraphics.GetRenderPass<ImGUIRenderPass>()->SetDestFrameBuffer("");

	aContext.myFrameBufferReadTextures[0] = {
		myReadFrameBuffer,
		PaintingFrameBuffer::kFinalColor,
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());
	aContext.myEnableDepthTest = false;

	aContext.myShouldClearColor = true;
	aContext.myClearColor[0] = 0;
	aContext.myClearColor[1] = 0;
	aContext.myClearColor[2] = 1;
	aContext.myClearColor[3] = 1;
}

void PainterAdapter::FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const
{
	const Source& data = static_cast<const Source&>(aData);

	aUB.SetUniform(0, 0, data.myCam.Get());
	aUB.SetUniform(1, 0, data.myColor);
	const glm::mat4& proj = data.myCam.GetProj();
	const glm::vec2 size(2 / proj[0][0], 2 / proj[1][1]);
	aUB.SetUniform(2, 0, size);
	aUB.SetUniform(3, 0, data.myTexSize);
	aUB.SetUniform(4, 0, data.myMousePosStart);
	aUB.SetUniform(5, 0, data.myMousePosEnd);
	const glm::vec2 gridCellSize = data.myTexSize / glm::vec2(data.myGridDims);
	aUB.SetUniform(6, 0, gridCellSize);
	aUB.SetUniform(7, 0, data.myPaintMode);
	aUB.SetUniform(8, 0, data.myBrushRadius);
}