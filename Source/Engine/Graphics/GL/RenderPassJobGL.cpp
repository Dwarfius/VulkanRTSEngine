#include "Precomp.h"
#include "RenderPassJobGL.h"

#include "Graphics/GL/GPUBufferGL.h"
#include "Graphics/GL/GraphicsGL.h"
#include "ModelGL.h"
#include "PipelineGL.h"
#include "TextureGL.h"
#include "Terrain.h"

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/UniformBlock.h>
#include <Graphics/Resources/Texture.h>

#include <Core/Profiler.h>

void RenderPassJobGL::OnInitialize(const RenderContext& aContext)
{
	// do nothing explicitly
}

void RenderPassJobGL::BindFrameBuffer(Graphics& aGraphics, const RenderContext& aContext)
{
	if (aContext.myFrameBuffer.empty())
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else
	{
		GraphicsGL& graphics = static_cast<GraphicsGL&>(aGraphics);
		FrameBufferGL& frameBuffer = graphics.GetFrameBufferGL(aContext.myFrameBuffer);
		frameBuffer.Bind();
	}
}

void RenderPassJobGL::Clear(const RenderContext& aContext)
{
	Profiler::ScopedMark profile("RenderPassJobGL::Clear");
	if (aContext.myScissorMode == RenderContext::ScissorMode::PerPass
		|| aContext.myScissorMode == RenderContext::ScissorMode::PerObject)
	{
		glEnable(GL_SCISSOR_TEST);
		if (aContext.myScissorMode == RenderContext::ScissorMode::PerPass)
		{
			// using bottom left
			glScissor(aContext.myScissorRect[0],
				aContext.myScissorRect[1],
				aContext.myScissorRect[2],
				aContext.myScissorRect[3]
			);
		}
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}

	if (aContext.myEnableBlending)
	{
		glEnable(GL_BLEND);

		const uint32_t blendEq = ConvertBlendEquation(aContext.myBlendingEq);
		glBlendEquation(blendEq);

		const uint32_t srcBlend = ConvertBlendMode(aContext.mySourceBlending);
		const uint32_t dstBlend = ConvertBlendMode(aContext.myDestinationBlending);
		glBlendFunc(srcBlend, dstBlend);

		const bool hasConstColor = aContext.mySourceBlending == RenderContext::Blending::ConstantColor
			|| aContext.mySourceBlending == RenderContext::Blending::OneMinusConstantColor
			|| aContext.mySourceBlending == RenderContext::Blending::ConstantAlpha
			|| aContext.mySourceBlending == RenderContext::Blending::OneMinusConstantAlpha
			|| aContext.myDestinationBlending == RenderContext::Blending::ConstantColor
			|| aContext.myDestinationBlending == RenderContext::Blending::OneMinusConstantColor
			|| aContext.myDestinationBlending == RenderContext::Blending::ConstantAlpha
			|| aContext.myDestinationBlending == RenderContext::Blending::OneMinusConstantAlpha;
		if (hasConstColor)
		{
			glBlendColor(aContext.myBlendColor[0],
				aContext.myBlendColor[1],
				aContext.myBlendColor[2],
				aContext.myBlendColor[3]);
		}
	}
	else
	{
		glDisable(GL_BLEND);
	}

	uint32_t clearMask = 0;
	if (aContext.myShouldClearColor)
	{
		clearMask |= GL_COLOR_BUFFER_BIT;
		glClearColor(aContext.myClearColor[0],
			aContext.myClearColor[1],
			aContext.myClearColor[2],
			aContext.myClearColor[3]);
	}
	if (aContext.myShouldClearDepth)
	{
		clearMask |= GL_DEPTH_BUFFER_BIT;
		glClearDepth(aContext.myClearDepth);
	}

	if (clearMask)
	{
		glClear(clearMask);
	}
}

void RenderPassJobGL::SetupContext(Graphics& aGraphics, const RenderContext& aContext)
{
	Profiler::ScopedMark profile("RenderPassJobGL::SetupContext");
	myCurrentPipeline = nullptr;
	myCurrentModel = nullptr;
	std::memset(myCurrentTextures, -1, sizeof(myCurrentTextures));

	myTextureSlotsUsed = 0;
	for (uint8_t i = 0; i < RenderContext::kFrameBufferReadTextures; i++)
	{
		const RenderContext::FrameBufferTexture& fbTexture = aContext.myFrameBufferReadTextures[i];
		if (!fbTexture.myFrameBuffer.empty())
		{
			glActiveTexture(GL_TEXTURE0 + i);
			GraphicsGL& graphics = static_cast<GraphicsGL&>(aGraphics);
			FrameBufferGL& frameBuffer = graphics.GetFrameBufferGL(fbTexture.myFrameBuffer);

			switch (fbTexture.myType)
			{
			case RenderContext::FrameBufferTexture::Type::Color:
				glBindTexture(GL_TEXTURE_2D, frameBuffer.GetColorTexture(fbTexture.myTextureInd));
				break;
			case RenderContext::FrameBufferTexture::Type::Depth:
				glBindTexture(GL_TEXTURE_2D, frameBuffer.GetDepthTexture());
				break;
			case RenderContext::FrameBufferTexture::Type::Stencil:
				glBindTexture(GL_TEXTURE_2D, frameBuffer.GetStencilTexture());
				break;
			default:
				ASSERT(false);
			}
			
			myTextureSlotsUsed++;
		}
	}

	ASSERT(aContext.myTextureCount <= RenderContext::kMaxObjectTextureSlots);
	for (uint8_t i = 0; i < aContext.myTextureCount; i++)
	{
		myTextureSlotsToUse[i] = aContext.myTexturesToActivate[i];
	}

	glViewport(aContext.myViewportOrigin[0], aContext.myViewportOrigin[1], 
		aContext.myViewportSize[0], aContext.myViewportSize[1]);

	if (aContext.myEnableDepthTest)
	{
		glEnable(GL_DEPTH_TEST);
		switch (aContext.myDepthTest)
		{
		case RenderContext::DepthTest::Never: glDepthFunc(GL_NEVER); break;
		case RenderContext::DepthTest::Less: glDepthFunc(GL_LESS); break;
		case RenderContext::DepthTest::Equal: glDepthFunc(GL_EQUAL); break;
		case RenderContext::DepthTest::LessOrEqual: glDepthFunc(GL_LEQUAL); break;
		case RenderContext::DepthTest::Greater: glDepthFunc(GL_GREATER); break;
		case RenderContext::DepthTest::NotEqual: glDepthFunc(GL_NOTEQUAL); break;
		case RenderContext::DepthTest::GreaterOrEqual: glDepthFunc(GL_GEQUAL); break;
		case RenderContext::DepthTest::Always: glDepthFunc(GL_ALWAYS); break;
		default: ASSERT_STR(false, "Unrecognized depth test!");
		}
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}

	if (aContext.myEnableCulling)
	{
		glEnable(GL_CULL_FACE);
		switch (aContext.myCulling)
		{
		case RenderContext::Culling::Front: glCullFace(GL_FRONT); break;
		case RenderContext::Culling::Back: glCullFace(GL_BACK); break;
		case RenderContext::Culling::FrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
		default: ASSERT_STR(false, "Unrecognized culling mode!");
		}
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	switch (aContext.myPolygonMode)
	{
	case RenderContext::PolygonMode::Point:	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
	case RenderContext::PolygonMode::Line:	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
	case RenderContext::PolygonMode::Fill:	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
	default: ASSERT_STR(false, "Unrecognized polygon mode!");
	}

	if (!aContext.myFrameBuffer.empty())
	{
		ASSERT(aContext.myFrameBufferDrawSlotsCount <= RenderContext::kMaxFrameBufferDrawSlots);
		uint32_t buffers[RenderContext::kMaxFrameBufferDrawSlots];
		for (uint8_t i = 0; i < aContext.myFrameBufferDrawSlotsCount; i++)
		{
			buffers[i] = aContext.myFrameBufferDrawSlots[i] == -1 ?
				GL_NONE : GL_COLOR_ATTACHMENT0 + aContext.myFrameBufferDrawSlots[i];
		}
		glDrawBuffers(aContext.myFrameBufferDrawSlotsCount, buffers);
	}
}

namespace
{
	template<class T>
	T GetCommand(std::span<const std::byte> aBytes, uint32_t& anIndex)
	{
		T cmd;
		std::memcpy(&cmd, &aBytes[anIndex], sizeof(T));
		anIndex += sizeof(T);
		return cmd;
	}

	uint32_t TranslatePrimitiveType(uint8_t aType)
	{
		switch (aType)
		{
		case IModel::PrimitiveType::Lines:
			return GL_LINES;
		case IModel::PrimitiveType::Triangles:
			return GL_TRIANGLES;
		}
		ASSERT(false);
		return GL_LINES;
	}
}

void RenderPassJobGL::RunCommands(const CmdBuffer& aCmdBuffer)
{
	Profiler::ScopedMark profile("RenderPassJobGL::RunCmdBuffer");
	std::span<const std::byte> bytes = aCmdBuffer.GetBuffer();
	uint32_t index = 0;

	while (index < bytes.size())
	{
		const uint8_t cmdId = static_cast<uint8_t>(bytes[index++]);
		switch (cmdId)
		{
		case RenderPassJob::SetPipelineCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::SetPipelineCmd>(bytes, index);

			PipelineGL* pipeline = static_cast<PipelineGL*>(cmd.myPipeline);
			ASSERT_STR(pipeline->GetState() == GPUResource::State::Valid
				|| pipeline->GetState() == GPUResource::State::PendingUnload,
				"Pipeline must be valid&up-to-date at this point!");
			if (pipeline != myCurrentPipeline)
			{
				pipeline->Bind();
				myCurrentPipeline = pipeline;
			}
			break;
		}
			
		case RenderPassJob::SetModelCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::SetModelCmd>(bytes, index);

			ModelGL* model = static_cast<ModelGL*>(cmd.myModel);
			ASSERT_STR(model->GetState() == GPUResource::State::Valid
				|| model->GetState() == GPUResource::State::PendingUnload,
				"Model must be valid&up-to-date at this point!");
			if (model != myCurrentModel)
			{
				model->Bind();
				myCurrentModel = model;
			}
			break;
		}
		case RenderPassJob::SetTextureCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::SetTextureCmd>(bytes, index);

			// TODO: Can we avoid this if? It's "lifting" myTextureSlotsUsed to user-space
			const int slotToUse = myTextureSlotsToUse[cmd.mySlot];
			if (slotToUse == -1)
			{
				continue;
			}

			TextureGL* texture = static_cast<TextureGL*>(cmd.myTexture);
			ASSERT_STR(texture->GetState() == GPUResource::State::Valid
				|| texture->GetState() == GPUResource::State::PendingUnload,
				"Texture must be valid&up-to-date at this point!");
			if (myCurrentTextures[slotToUse] != texture)
			{
				glActiveTexture(GL_TEXTURE0 + myTextureSlotsUsed + slotToUse);
				texture->Bind();
				myCurrentTextures[slotToUse] = texture;
			}
			break;
		}
			
		case RenderPassJob::SetBufferCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::SetBufferCmd>(bytes, index);

			// Now we can update the uniform blocks
			GPUBufferGL& buffer = *static_cast<GPUBufferGL*>(cmd.myBuffer);
			ASSERT_STR(buffer.GetState() == GPUResource::State::Valid
				|| buffer.GetState() == GPUResource::State::PendingUnload,
				"UBO must be valid at this point!");
			// TODO: implement logic that doesn't rebind same slots:
			// If pipeline A has X, Y, Z uniform blocks
			// and pipeline B has X, V, W,
			// if we bind from A to B (or vice versa), no need to rebind X
			buffer.Bind(cmd.mySlot);
			break;
		}
		case RenderPassJob::DrawIndexedCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::DrawIndexedCmd>(bytes, index);

			// TODO: bake it into cmd!
			const uint32_t drawMode = myCurrentModel->GetDrawMode();
			const uint32_t primitiveCount = cmd.myCount;
			const size_t indexOffset = cmd.myOffset * sizeof(IModel::IndexType);
			glDrawElements(drawMode,
				static_cast<GLsizei>(primitiveCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<void*>(indexOffset)
			);
			break;
		}
		case RenderPassJob::SetScissorRectCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::SetScissorRectCmd>(bytes, index);
			glScissor(static_cast<GLsizei>(cmd.myRect[0]),
				static_cast<GLsizei>(cmd.myRect[1]),
				static_cast<GLsizei>(cmd.myRect[2]),
				static_cast<GLsizei>(cmd.myRect[3])
			);
			break;
		}
		case RenderPassJob::DrawTesselatedCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::DrawTesselatedCmd>(bytes, index);
			glDrawArraysInstanced(GL_PATCHES, cmd.myOffset, cmd.myCount, cmd.myInstanceCount);
			break;
		}
		case RenderPassJob::SetTesselationPatchCPs::kId:
		{
			auto cmd = GetCommand<RenderPassJob::SetTesselationPatchCPs>(bytes, index);
			glPatchParameteri(GL_PATCH_VERTICES, cmd.myControlPointCount);
			break;
		}
		case RenderPassJob::DrawArrayCmd::kId:
		{
			auto cmd = GetCommand<RenderPassJob::DrawArrayCmd>(bytes, index);
			glDrawArrays(TranslatePrimitiveType(cmd.myDrawMode), cmd.myOffset, cmd.myCount);
			break;
		}
		case RenderPassJob::DispatchCompute::kId:
		{
			auto cmd = GetCommand<RenderPassJob::DispatchCompute>(bytes, index);
			glDispatchCompute(cmd.myGroupsX, cmd.myGroupsY, cmd.myGroupsZ);
			break;
		}
		case RenderPassJob::MemBarrier::kId:
		{
			auto cmd = GetCommand<RenderPassJob::MemBarrier>(bytes, index);
			GLbitfield barrier = 0;
			if ((cmd.myType & MemBarrierType::UniformBuffer) == MemBarrierType::UniformBuffer)
			{
				barrier |= GL_UNIFORM_BARRIER_BIT;
			}
			if ((cmd.myType & MemBarrierType::ShaderStorageBuffer) == MemBarrierType::ShaderStorageBuffer)
			{
				barrier |= GL_SHADER_STORAGE_BARRIER_BIT;
			}
			if ((cmd.myType & MemBarrierType::CommandBuffer) == MemBarrierType::CommandBuffer)
			{
				barrier |= GL_COMMAND_BARRIER_BIT;
			}
			glMemoryBarrier(barrier);
			break;
		}
		case RenderPassJob::DrawIndexedInstanced::kId:
		{
			auto cmd = GetCommand<RenderPassJob::DrawIndexedInstanced>(bytes, index);
			const uint32_t drawMode = myCurrentModel->GetDrawMode();
			const size_t indexOffset = cmd.myOffset * sizeof(IModel::IndexType);
			glDrawElementsInstanced(drawMode, 
				static_cast<GLsizei>(cmd.myCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<void*>(indexOffset),
				cmd.myInstanceCount);
			break;
		}
		default:
			ASSERT_STR(false, "Unknown command!");
		}
	}
	ASSERT_STR(index == bytes.size(), "We somehow read more than we wrote!");
}

void RenderPassJobGL::DownloadFrameBuffer(Graphics& aGraphics, Texture& aTexture)
{
	const RenderContext& context = GetRenderContext();
	const uint32_t width = context.myViewportSize[0];
	const uint32_t height = context.myViewportSize[1];

	Texture::Format format;
	if (context.myFrameBuffer.empty())
	{
		format = Texture::Format::SNorm_RGB;
	}
	else
	{
		const FrameBuffer& buffer = aGraphics.GetNamedFrameBuffer(
			context.myFrameBuffer
		);
		format = buffer.myColors[0].myFormat;
	}
	const size_t pixelSize = TextureGL::GetPixelDataTypeSize(format);
	
	unsigned char* buffer = new unsigned char[width * height * pixelSize];
	aTexture.SetWidth(width);
	aTexture.SetHeight(height);
	aTexture.SetFormat(format);
	aTexture.SetPixels(buffer);

	// TODO: this forces a stall on the GPU - use mapped buffers instead
	const uint32_t x = context.myViewportOrigin[0];
	const uint32_t y = context.myViewportOrigin[1];
	glReadPixels(x, y, width, height,
		TextureGL::TranslateFormat(format),
		TextureGL::DeterminePixelDataType(format),
		buffer
	);
}

constexpr uint32_t RenderPassJobGL::ConvertBlendMode(RenderContext::Blending blendMode)
{
	switch (blendMode)
	{
	case RenderContext::Blending::Zero: return GL_ZERO;
	case RenderContext::Blending::One: return GL_ONE;
	case RenderContext::Blending::SourceColor: return GL_SRC_COLOR;
	case RenderContext::Blending::OneMinusSourceColor: return GL_ONE_MINUS_SRC_COLOR;
	case RenderContext::Blending::DestinationColor: return GL_DST_COLOR;
	case RenderContext::Blending::OneMinusDestinationColor: return GL_ONE_MINUS_DST_COLOR;
	case RenderContext::Blending::SourceAlpha: return GL_SRC_ALPHA;
	case RenderContext::Blending::OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
	case RenderContext::Blending::DestinationAlpha: return GL_DST_ALPHA;
	case RenderContext::Blending::OneMinusDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
	case RenderContext::Blending::ConstantColor: return GL_CONSTANT_COLOR;
	case RenderContext::Blending::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
	case RenderContext::Blending::ConstantAlpha: return GL_CONSTANT_ALPHA;
	case RenderContext::Blending::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
	default: ASSERT_STR(false, "Unrecognized blend mode!"); return 0;
	}
}

constexpr uint32_t RenderPassJobGL::ConvertBlendEquation(RenderContext::BlendingEq aBlendEq)
{
	switch (aBlendEq)
	{
	case RenderContext::BlendingEq::Add: return GL_FUNC_ADD;
	case RenderContext::BlendingEq::Substract: return GL_FUNC_SUBTRACT;
	case RenderContext::BlendingEq::ReverseSubstract: return GL_FUNC_REVERSE_SUBTRACT;
	case RenderContext::BlendingEq::Min: return GL_MIN;
	case RenderContext::BlendingEq::Max: return GL_MAX;
	default: ASSERT_STR(false, "Unrecognized blend equation!"); return 0;
	}
}