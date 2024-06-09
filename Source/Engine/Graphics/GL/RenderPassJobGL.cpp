#include "Precomp.h"
#include "RenderPassJobGL.h"

#include "Graphics/GL/UniformBufferGL.h"
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

void RenderPassJobGL::RunJobs(StableVector<RenderJob>& aJobs)
{
	Profiler::ScopedMark profile("RenderPassJobGL::RunJobs");
	aJobs.ForEach([&](RenderJob& r){
		Profiler::ScopedMark profile("RenderPassJobGL::RenderJob");

		// TODO: unlikely
		if (GetRenderContext().myScissorMode == RenderContext::ScissorMode::PerObject)
		{
			glm::ivec4 scissorRect = r.GetScissorRect();
			glScissor(static_cast<GLsizei>(scissorRect.x),
				static_cast<GLsizei>(scissorRect.y),
				static_cast<GLsizei>(scissorRect.z),
				static_cast<GLsizei>(scissorRect.w)
			);
		}

		PipelineGL* pipeline = static_cast<PipelineGL*>(r.GetPipeline());
		ASSERT_STR(pipeline->GetState() == GPUResource::State::Valid
			|| pipeline->GetState() == GPUResource::State::PendingUnload,
			"Pipeline must be valid&up-to-date at this point!");
		if (pipeline != myCurrentPipeline)
		{
			pipeline->Bind();
			myCurrentPipeline = pipeline;
		}

		// Now we can update the uniform blocks
		size_t adapterCount = pipeline->GetAdapterCount();
		RenderJob::UniformSet& uniforms = r.GetUniformSet();
		ASSERT_STR(adapterCount < std::numeric_limits<uint32_t>::max(),
			"Number of UBO doesn't fit for binding!");
		for (size_t i = 0; i < adapterCount; i++)
		{
			UniformBufferGL& buffer = *static_cast<UniformBufferGL*>(uniforms[i]);
			ASSERT_STR(buffer.GetState() == GPUResource::State::Valid
				|| buffer.GetState() == GPUResource::State::PendingUnload,
				"UBO must be valid at this point!");
			// TODO: implement logic that doesn't rebind same slots:
			// If pipeline A has X, Y, Z uniform blocks
			// and pipeline B has X, V, W,
			// if we bind from A to B (or vice versa), no need to rebind X
			buffer.Bind(static_cast<uint32_t>(i));
		}

		ModelGL* model = static_cast<ModelGL*>(r.GetModel());
		if (model) [[likely]]
		{
			ASSERT_STR(model->GetState() == GPUResource::State::Valid
			|| model->GetState() == GPUResource::State::PendingUnload,
				"Model must be valid&up-to-date at this point!");
			if (model != myCurrentModel)
			{
				model->Bind();
				myCurrentModel = model;
			}
		}

		RenderJob::TextureSet& textures = r.GetTextures();
		for (size_t textureInd = 0; textureInd < textures.GetSize(); textureInd++)
		{
			int slotToUse = myTextureSlotsToUse[textureInd];
			if (slotToUse == -1)
			{
				continue;
			}

			TextureGL* texture = static_cast<TextureGL*>(textures[textureInd]);
			ASSERT_STR(texture->GetState() == GPUResource::State::Valid
				|| texture->GetState() == GPUResource::State::PendingUnload,
				"Texture must be valid&up-to-date at this point!");
			if (myCurrentTextures[slotToUse] != texture)
			{
				glActiveTexture(GL_TEXTURE0 + myTextureSlotsUsed + slotToUse);
				texture->Bind();
				myCurrentTextures[slotToUse] = texture;
			}
		}

		switch (r.GetDrawMode())
		{
		case RenderJob::DrawMode::Indexed:
		{
			const uint32_t drawMode = model->GetDrawMode();
			const RenderJob::IndexedDrawParams& params = r.GetDrawParams().myIndexedParams;
			const uint32_t primitiveCount = params.myCount;
			ASSERT_STR(primitiveCount < static_cast<uint32_t>(std::numeric_limits<GLsizei>::max()), 
				"Exceeded the limit of primitives!");
			const size_t indexOffset = params.myOffset * sizeof(IModel::IndexType);
			glDrawElements(drawMode, 
				static_cast<GLsizei>(primitiveCount), 
				GL_UNSIGNED_INT, 
				reinterpret_cast<void*>(indexOffset)
			);
			break;
		}
		case RenderJob::DrawMode::Tesselated:
		{
			// number of control points per patch is hardcoded for now
			// Terrain uses 1 CP to expand it into a quad
			glPatchParameteri(GL_PATCH_VERTICES, 1);

			const int instances = r.GetDrawParams().myTessParams.myInstanceCount;
			glDrawArraysInstanced(GL_PATCHES, 0, 1, instances);
			break;
		}
		case RenderJob::DrawMode::Array:
		{
			const uint32_t drawMode = model->GetDrawMode();
			const RenderJob::ArrayDrawParams& params = r.GetDrawParams().myArrayParams;
			glDrawArrays(drawMode, params.myOffset, params.myCount);
			break;
		}
		default:
			ASSERT(false);
			break;
		}
	});
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
			// TODO: factor this blurb out
			RenderPassJob::SetPipelineCmd cmd;
			std::memcpy(&cmd, &bytes[index], sizeof(cmd));
			index += sizeof(cmd);

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
			RenderPassJob::SetModelCmd cmd;
			std::memcpy(&cmd, &bytes[index], sizeof(cmd));
			index += sizeof(cmd);

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
			RenderPassJob::SetTextureCmd cmd;
			std::memcpy(&cmd, &bytes[index], sizeof(cmd));
			index += sizeof(cmd);

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
			
		case RenderPassJob::SetUniformBufferCmd::kId:
		{
			RenderPassJob::SetUniformBufferCmd cmd;
			std::memcpy(&cmd, &bytes[index], sizeof(cmd));
			index += sizeof(cmd);

			// Now we can update the uniform blocks
			UniformBufferGL& buffer = *static_cast<UniformBufferGL*>(cmd.myUniformBuffer);
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
			RenderPassJob::DrawIndexedCmd cmd;
			std::memcpy(&cmd, &bytes[index], sizeof(cmd));
			index += sizeof(cmd);

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
			RenderPassJob::SetScissorRectCmd cmd;
			std::memcpy(&cmd, &bytes[index], sizeof(cmd));
			index += sizeof(cmd);

			glScissor(static_cast<GLsizei>(cmd.myRect[0]),
				static_cast<GLsizei>(cmd.myRect[1]),
				static_cast<GLsizei>(cmd.myRect[2]),
				static_cast<GLsizei>(cmd.myRect[3])
			);
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