#include "Precomp.h"
#include "PaintingRenderPass.h"

#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUIRendering.h>
#include <Engine/Graphics/Adapters/ObjectMatricesAdapter.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/Camera.h>

void PaintingRenderPass::SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics)
{
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	aPipeline->ExecLambdaOnLoad([this, &aGraphics](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		const UniformAdapter& adapter = pipeline->GetAdapter(0);
		myBuffer = aGraphics.CreateUniformBuffer(adapter.GetDescriptor().GetBlockSize());
	});
}

void PaintingRenderPass::SetParams(const PaintParams& aParams)
{
#ifdef ASSERT_MUTEX
	AssertLock lock(myParamsMutex);
#endif
	myParams = aParams;
}

std::string_view PaintingRenderPass::GetWriteBuffer() const
{
	return myWriteToOther ?
		static_cast<std::string_view>(OtherPaintingFrameBuffer::kName) :
		static_cast<std::string_view>(PaintingFrameBuffer::kName);
}

std::string_view PaintingRenderPass::GetReadBuffer() const
{
	return !myWriteToOther ?
		static_cast<std::string_view>(OtherPaintingFrameBuffer::kName) :
		static_cast<std::string_view>(PaintingFrameBuffer::kName);
}

PaintParams PaintingRenderPass::GetParams() const
{
#ifdef ASSERT_MUTEX
	AssertLock lock(myParamsMutex);
#endif
	return myParams;
}

void PaintingRenderPass::Execute(Graphics& aGraphics)
{
	RenderPass::Execute(aGraphics);

	if (!myPipeline.IsValid() 
		|| myPipeline->GetState() != GPUResource::State::Valid 
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid 
		|| myBuffer->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	PaintParams paintParams = GetParams();
	RenderJob& job = AllocateJob();
	job.SetPipeline(myPipeline.Get());
	job.SetModel(aGraphics.GetFullScreenQuad().Get());
	if (paintParams.myPaintTexture.IsValid())
	{
		job.GetTextures().PushBack(paintParams.myPaintTexture.Get());
	}
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);

	glm::vec2 scaledSize(1.f);
	if (paintParams.myPaintTexture.IsValid())
	{
		glm::vec2 size = paintParams.myPaintTexture->GetSize();
		scaledSize = size / paintParams.myPaintInverseScale;
	}

	PainterAdapter::Source source{
		aGraphics,
		paintParams.myCamera,
		paintParams.myColor,
		paintParams.myTexSize,
		paintParams.myPrevMousePos,
		paintParams.myMousePos,
		paintParams.myGridDims,
		scaledSize,
		paintParams.myPaintMode,
		paintParams.myBrushSize
	};
	PainterAdapter adapter;
	UniformBlock block(*myBuffer.Get(), adapter.ourDescriptor);
	adapter.FillUniformBlock(source, block);
	job.GetUniformSet().PushBack(myBuffer.Get());

	myWriteToOther = !myWriteToOther;
}

void PaintingRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = GetWriteBuffer();
	aContext.myFrameBufferDrawSlots[0] = PaintingFrameBuffer::kFinalColor;
	aContext.myFrameBufferDrawSlots[1] = PaintingFrameBuffer::kPaintingColor;
	aGraphics.GetRenderPass<DisplayRenderPass>()->SetReadBuffer(GetWriteBuffer());

	aContext.myFrameBufferReadTextures[0] = {
		GetReadBuffer(),
		PaintingFrameBuffer::kPaintingColor,
		RenderContext::FrameBufferTexture::Type::Color
	};

	const PaintParams paintParams = GetParams();
	const int width = static_cast<int>(paintParams.myTexSize.x);
	const int height = static_cast<int>(paintParams.myTexSize.y);
	aGraphics.ResizeNamedFrameBuffer(GetWriteBuffer(), glm::ivec2(width, height));
	aContext.myViewportSize[0] = width;
	aContext.myViewportSize[1] = height;
	aContext.myEnableDepthTest = false;

	aContext.myTexturesToActivate[0] = paintParams.myPaintTexture.IsValid() ? 0 : -1;
}

void DisplayRenderPass::SetPipeline(Handle<Pipeline> aPipeline, Graphics& aGraphics)
{
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	aPipeline->ExecLambdaOnLoad([this, &aGraphics](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		const UniformAdapter& adapter = pipeline->GetAdapter(0);
		myBuffer = aGraphics.CreateUniformBuffer(adapter.GetDescriptor().GetBlockSize());
	});
}

void DisplayRenderPass::SetParams(const PaintParams& aParams)
{
#ifdef ASSERT_MUTEX
	AssertLock lock(myParamsMutex);
#endif
	myParams = aParams;
}

PaintParams DisplayRenderPass::GetParams() const
{
#ifdef ASSERT_MUTEX
	AssertLock lock(myParamsMutex);
#endif
	return myParams;
}

void DisplayRenderPass::Execute(Graphics& aGraphics)
{
	RenderPass::Execute(aGraphics);

	if (!myPipeline.IsValid()
		|| myPipeline->GetState() != GPUResource::State::Valid
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid
		|| myBuffer->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderJob& job = AllocateJob();
	job.SetPipeline(myPipeline.Get());
	job.SetModel(aGraphics.GetFullScreenQuad().Get());

	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);

	const PaintParams paintParams = GetParams();
	PainterAdapter::Source source{
		aGraphics,
		paintParams.myCamera,
		paintParams.myColor,
		paintParams.myTexSize,
		paintParams.myPrevMousePos,
		paintParams.myMousePos,
		paintParams.myGridDims,
		glm::vec2(),
		paintParams.myPaintMode,
		paintParams.myBrushSize
	};
	PainterAdapter adapter;
	UniformBlock block(*myBuffer.Get(), adapter.ourDescriptor);
	adapter.FillUniformBlock(source, block);
	job.GetUniformSet().PushBack(myBuffer.Get());
}

void DisplayRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
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

void PainterAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
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
	aUB.SetUniform(7, 0, data.myPaintSizeScaled);
	aUB.SetUniform(8, 0, static_cast<int>(data.myPaintMode));
	aUB.SetUniform(9, 0, data.myBrushRadius);
}