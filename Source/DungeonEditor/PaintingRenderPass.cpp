#include "Precomp.h"
#include "PaintingRenderPass.h"

#include <Engine/Input.h>

#include <Engine/Graphics/Adapters/ObjectMatricesAdapter.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/Camera.h>

void PaintingRenderPass::SetPipelines(Handle<Pipeline> aPaintPipeline,
	Handle<Pipeline> aDisplayPipeline, Graphics& aGraphics)
{
	myPaintPipeline = aGraphics.GetOrCreate(aPaintPipeline).Get<GPUPipeline>();
	aPaintPipeline->ExecLambdaOnLoad([this, &aGraphics](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		const UniformAdapter& adapter = pipeline->GetAdapter(0);
		myPaintBuffer = aGraphics.CreateUniformBuffer(adapter.GetDescriptor().GetBlockSize());
	});

	myDisplayPipeline = aGraphics.GetOrCreate(aDisplayPipeline).Get<GPUPipeline>();
	aDisplayPipeline->ExecLambdaOnLoad([this, &aGraphics](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		const UniformAdapter& adapter = pipeline->GetAdapter(0);
		myDisplayBuffer = aGraphics.CreateUniformBuffer(adapter.GetDescriptor().GetBlockSize());
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

	if (!myPaintPipeline.IsValid() 
		|| !myDisplayPipeline.IsValid()
		|| myPaintPipeline->GetState() != GPUResource::State::Valid
		|| myDisplayPipeline->GetState() != GPUResource::State::Valid
		|| aGraphics.GetFullScreenQuad()->GetState() != GPUResource::State::Valid 
		|| myPaintBuffer->GetState() != GPUResource::State::Valid
		|| myDisplayBuffer->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	ExecutePainting(aGraphics);
	ExecuteDisplay(aGraphics);

	myWriteToOther = !myWriteToOther;
}

void PaintingRenderPass::ExecutePainting(Graphics& aGraphics)
{
	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreatePaintContext(aGraphics));
	PaintParams paintParams = GetParams();
	RenderJob& job = passJob.AllocateJob();
	job.SetPipeline(myPaintPipeline.Get());
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
	UniformBlock block(*myPaintBuffer.Get(), adapter.ourDescriptor);
	adapter.FillUniformBlock(source, block);
	job.GetUniformSet().PushBack(myPaintBuffer.Get());
}

void PaintingRenderPass::ExecuteDisplay(Graphics& aGraphics)
{
	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateDisplayContext(aGraphics));

	RenderJob& job = passJob.AllocateJob();
	job.SetPipeline(myDisplayPipeline.Get());
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
	UniformBlock block(*myDisplayBuffer.Get(), adapter.ourDescriptor);
	adapter.FillUniformBlock(source, block);
	job.GetUniformSet().PushBack(myDisplayBuffer.Get());
}

RenderContext PaintingRenderPass::CreatePaintContext(Graphics& aGraphics) const
{
	const PaintParams paintParams = GetParams();
	const int width = static_cast<int>(paintParams.myTexSize.x);
	const int height = static_cast<int>(paintParams.myTexSize.y);
	aGraphics.ResizeNamedFrameBuffer(GetWriteBuffer(), glm::ivec2(width, height));
	
	return {
		.myFrameBufferReadTextures = {
			{
				GetReadBuffer(),
				PaintingFrameBuffer::kPaintingColor,
				RenderContext::FrameBufferTexture::Type::Color
			}
		},
		.myFrameBuffer = GetWriteBuffer(),
		.myTextureCount = paintParams.myPaintTexture.IsValid() ? 1u : 0u,
		.myTexturesToActivate = {
			paintParams.myPaintTexture.IsValid() ? 0 : -1
		},
		.myFrameBufferDrawSlotsCount = 2u,
		.myFrameBufferDrawSlots = { 
			PaintingFrameBuffer::kFinalColor, 
			PaintingFrameBuffer::kPaintingColor
		},
		.myViewportSize = {width, height},
		.myEnableDepthTest = false
	};
}

RenderContext PaintingRenderPass::CreateDisplayContext(Graphics& aGraphics) const
{
	return {
		.myFrameBufferReadTextures = {
			{
				GetWriteBuffer(),
				PaintingFrameBuffer::kFinalColor,
				RenderContext::FrameBufferTexture::Type::Color
			}
		},
		.myFrameBuffer = "",
		.myClearColor = { 0, 0, 1, 1 },
		.myViewportSize = {
			static_cast<int>(aGraphics.GetWidth()),
			static_cast<int>(aGraphics.GetHeight())
		},
		.myShouldClearColor = true,
	};
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