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
#include <Graphics/Resources/GPUBuffer.h>
#include <Graphics/Camera.h>

#include <Core/Profiler.h>

void PaintingRenderPass::SetPipelines(Handle<Pipeline> aPaintPipeline,
	Handle<Pipeline> aDisplayPipeline, Graphics& aGraphics)
{
	myPaintPipeline = aGraphics.GetOrCreate(aPaintPipeline).Get<GPUPipeline>();
	aPaintPipeline->ExecLambdaOnLoad([this, &aGraphics](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		const UniformAdapter& adapter = pipeline->GetAdapter(0);
		myPaintBuffer = aGraphics.CreateUBOBuffer(adapter.GetDescriptor().GetBlockSize());
	});

	myDisplayPipeline = aGraphics.GetOrCreate(aDisplayPipeline).Get<GPUPipeline>();
	aDisplayPipeline->ExecLambdaOnLoad([this, &aGraphics](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		const UniformAdapter& adapter = pipeline->GetAdapter(0);
		myDisplayBuffer = aGraphics.CreateUBOBuffer(adapter.GetDescriptor().GetBlockSize());
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
	Profiler::ScopedMark mark("IDRenderPass::Execute");
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
	PaintParams paintParams = GetParams();
	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreatePaintContext(aGraphics));
	CmdBuffer& cmdBuffer = passJob.GetCmdBuffer();
	cmdBuffer.Clear();

	RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
	pipelineCmd.myPipeline = myPaintPipeline.Get();

	RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd>();
	modelCmd.myModel = aGraphics.GetFullScreenQuad().Get();

	glm::vec2 scaledSize(1.f);
	if (paintParams.myPaintTexture.IsValid())
	{
		RenderPassJob::SetTextureCmd& textureCmd = cmdBuffer.Write<RenderPassJob::SetTextureCmd>();
		textureCmd.mySlot = 0;
		textureCmd.myTexture = paintParams.myPaintTexture.Get();

		const glm::vec2 size = paintParams.myPaintTexture->GetSize();
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
	UniformBlock block(*myPaintBuffer.Get());
	adapter.FillUniformBlock(source, block);

	RenderPassJob::SetBufferCmd& uboCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	uboCmd.mySlot = PainterAdapter::kBindpoint;
	uboCmd.myBuffer = myPaintBuffer.Get();

	RenderPassJob::DrawArrayCmd& drawCmd = cmdBuffer.Write<RenderPassJob::DrawArrayCmd>();
	drawCmd.myDrawMode = IModel::PrimitiveType::Triangles;
	drawCmd.myOffset = 0;
	drawCmd.myCount = 6;
}

void PaintingRenderPass::ExecuteDisplay(Graphics& aGraphics)
{
	const PaintParams paintParams = GetParams();
	RenderPassJob& passJob = aGraphics.CreateRenderPassJob(CreateDisplayContext(aGraphics));
	CmdBuffer& cmdBuffer = passJob.GetCmdBuffer();
	cmdBuffer.Clear();

	RenderPassJob::SetPipelineCmd& pipelineCmd = cmdBuffer.Write<RenderPassJob::SetPipelineCmd>();
	pipelineCmd.myPipeline = myDisplayPipeline.Get();

	RenderPassJob::SetModelCmd& modelCmd = cmdBuffer.Write<RenderPassJob::SetModelCmd>();
	modelCmd.myModel = aGraphics.GetFullScreenQuad().Get();

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
	UniformBlock block(*myDisplayBuffer.Get());
	adapter.FillUniformBlock(source, block);
	RenderPassJob::SetBufferCmd& uboCmd = cmdBuffer.Write<RenderPassJob::SetBufferCmd>();
	uboCmd.mySlot = PainterAdapter::kBindpoint;
	uboCmd.myBuffer = myDisplayBuffer.Get();

	RenderPassJob::DrawArrayCmd& drawCmd = cmdBuffer.Write<RenderPassJob::DrawArrayCmd>();
	drawCmd.myDrawMode = IModel::PrimitiveType::Triangles;
	drawCmd.myOffset = 0;
	drawCmd.myCount = 6;
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

	aUB.SetUniform(ourDescriptor.GetOffset(0, 0), data.myCam.Get());
	aUB.SetUniform(ourDescriptor.GetOffset(1, 0), data.myColor);
	const glm::mat4& proj = data.myCam.GetProj();
	const glm::vec2 size(2 / proj[0][0], 2 / proj[1][1]);
	aUB.SetUniform(ourDescriptor.GetOffset(2, 0), size);
	aUB.SetUniform(ourDescriptor.GetOffset(3, 0), data.myTexSize);
	aUB.SetUniform(ourDescriptor.GetOffset(4, 0), data.myMousePosStart);
	aUB.SetUniform(ourDescriptor.GetOffset(5, 0), data.myMousePosEnd);
	const glm::vec2 gridCellSize = data.myTexSize / glm::vec2(data.myGridDims);
	aUB.SetUniform(ourDescriptor.GetOffset(6, 0), gridCellSize);
	aUB.SetUniform(ourDescriptor.GetOffset(7, 0), data.myPaintSizeScaled);
	aUB.SetUniform(ourDescriptor.GetOffset(8, 0), static_cast<int>(data.myPaintMode));
	aUB.SetUniform(ourDescriptor.GetOffset(9, 0), data.myBrushRadius);
}