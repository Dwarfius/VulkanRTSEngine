#include "Precomp.h"
#include "PaintingRenderPass.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Camera.h>
#include <Engine/Input.h>

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

	const glm::vec2 texSize(aGraphics.GetWidth(), aGraphics.GetHeight());
	
	glm::vec2 mousePos = Input::GetMousePos();
	mousePos.y = aGraphics.GetHeight() - mousePos.y;
	
	int paintMode = 0; // nothing
	if (Input::GetMouseBtn(0))
	{
		paintMode = 1; // paint
	}
	else if (Input::GetMouseBtn(1))
	{
		paintMode = 2; // erase under mouse
	}
	else if (Input::GetKeyPressed(Input::Keys::Space))
	{
		paintMode = -1; // erase all
	}

	myBrushRadius += Input::GetMouseWheelDelta() / 300.f;
	myBrushRadius = glm::clamp(myBrushRadius, 0.f, 1.f);

	Camera dudCamera(aGraphics.GetWidth(), aGraphics.GetHeight());
	PainterAdapter::Source source{ 
		dudCamera, 
		texSize,
		mousePos,
		paintMode,
		myBrushRadius
	};
	PainterAdapter adapter;
	adapter.FillUniformBlock(source, *myBlock);
	job.AddUniformBlock(*myBlock);

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();
	passJob.Add(job);

	myWriteToOther = !myWriteToOther;
}

void PaintingRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myFrameBuffer = GetWriteBuffer();

	aContext.myFrameBufferReadTextures[0] = {
		myWriteToOther ?
			static_cast<std::string_view>(PaintingFrameBuffer::kName) :
			static_cast<std::string_view>(OtherPaintingFrameBuffer::kName),
		uint8_t(0),
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());
	aContext.myEnableDepthTest = false;
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

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();
	passJob.Add(job);
}

void DisplayRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myFrameBuffer = ""; 

	aContext.myFrameBufferReadTextures[0] = {
		myPass->GetWriteBuffer(),
		uint8_t(1),
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());
	aContext.myEnableDepthTest = false;
}

void PainterAdapter::FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const
{
	const Source& data = static_cast<const Source&>(aData);

	aUB.SetUniform(0, 0, data.myTexSize);
	aUB.SetUniform(1, 0, data.myMousePos);
	aUB.SetUniform(2, 0, data.myPaintMode);
	aUB.SetUniform(3, 0, data.myBrushRadius);
}