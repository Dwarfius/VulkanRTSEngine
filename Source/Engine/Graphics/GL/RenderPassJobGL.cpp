#include "Precomp.h"
#include "RenderPassJobGL.h"

#include "PipelineGL.h"
#include "ModelGL.h"
#include "TextureGL.h"

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/UniformBlock.h>
#include <Graphics/Resources/Texture.h>

#include <Core/Profiler.h>

#include "Graphics/GL/UniformBufferGL.h"
#include "../../Terrain.h"

void RenderPassJobGL::Add(const RenderJob& aJob)
{
	myJobs.push_back(aJob);
}

void RenderPassJobGL::AddRange(std::vector<RenderJob>&& aJobs)
{
	myJobs = std::move(aJobs);
}

bool RenderPassJobGL::HasWork() const
{
	return !myJobs.empty();
}

void RenderPassJobGL::OnInitialize(const RenderContext& aContext)
{
	// do nothing explicitly
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

void RenderPassJobGL::SetupContext(const RenderContext& aContext)
{
	Profiler::ScopedMark profile("RenderPassJobGL::SetupContext");
	myCurrentPipeline = nullptr;
	myCurrentModel = nullptr;
	std::memset(myCurrentTextures, -1, sizeof(myCurrentTextures));

	myTexturesToUse = 0;
	for (uint32_t i = 0; i < RenderContext::kMaxTextureActivations; i++)
	{
		myTextureSlotsToUse[i] = aContext.myTexturesToActivate[i];
		if (myTextureSlotsToUse[i] != -1)
		{
			myTexturesToUse++;
		}
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

	switch (aContext.myPolygonMode)
	{
	case RenderContext::PolygonMode::Point:	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
	case RenderContext::PolygonMode::Line:	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
	case RenderContext::PolygonMode::Fill:	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
	default: ASSERT_STR(false, "Unrecognized polygon mode!");
	}
}

void RenderPassJobGL::RunJobs()
{
	Profiler::ScopedMark profile("RenderPassJobGL::RunJobs");
	for (RenderJob& r : myJobs)
	{
		Profiler::ScopedMark profile("RenderPassJobGL::RenderJob");
		if (r.HasLastHandles())
		{
			// one of the handles has expired, meaning noone needs it anymore
			// cheaper to skip it
			continue;
		}

		// TODO: unlikely
		if (GetRenderContext().myScissorMode == RenderContext::ScissorMode::PerObject)
		{
			glm::vec4 scissorRect = r.GetScissorRect();
			glScissor(static_cast<GLsizei>(scissorRect.x),
				static_cast<GLsizei>(scissorRect.y),
				static_cast<GLsizei>(scissorRect.z),
				static_cast<GLsizei>(scissorRect.w)
			);
		}

		PipelineGL* pipeline = r.GetPipeline().Get<PipelineGL>();
		ModelGL* model = r.GetModel().Get<ModelGL>();

		if (pipeline != myCurrentPipeline)
		{
			pipeline->Bind();
			myCurrentPipeline = pipeline;

			// binding uniform blocks to according slots
			size_t blockCount = pipeline->GetUBOCount();
			ASSERT_STR(blockCount < std::numeric_limits<uint32_t>::max(), "Index of UBO block doesn't fit for binding!");
			for (size_t i = 0; i < blockCount; i++)
			{
				// TODO: implement logic that doesn't rebind same slots:
				// If pipeline A has X, Y, Z uniform blocks
				// and pipeline B has X, V, W,
				// if we bind from A to B (or vice versa), no need to rebind X
				pipeline->GetUBO(i).Bind(static_cast<uint32_t>(i));
			}
		}

		RenderJob::TextureSet& textures = r.GetTextures();
		for (size_t textureInd = 0; textureInd < textures.size(); textureInd++)
		{
			int slotToUse = myTextureSlotsToUse[textureInd];
			if (slotToUse == -1)
			{
				continue;
			}

			TextureGL* texture = textures[textureInd].Get<TextureGL>();
			if (myCurrentTextures[slotToUse] != texture)
			{
				glActiveTexture(GL_TEXTURE0 + slotToUse);
				texture->Bind();
				myCurrentTextures[slotToUse] = texture;
			}
		}

		if (model != myCurrentModel)
		{
			model->Bind();
			myCurrentModel = model;
		}

		// Now we can update the uniform blocks
		size_t descriptorCount = pipeline->GetDescriptorCount();
		RenderJob::UniformSet& uniforms = r.GetUniformSet();
		for (size_t i = 0; i < descriptorCount; i++)
		{
			// grabbing the descriptor because it has the locations
			// of uniform values to be uploaded to
			const Descriptor& aDesc = pipeline->GetDescriptor(i);
			// there's a UBO for every descriptor of the pipeline
			UniformBufferGL& ubo = pipeline->GetUBO(i);
			UniformBufferGL::UploadDescriptor uploadDesc;
			uploadDesc.mySize = aDesc.GetBlockSize(); // TODO: uniform buffer already has a descriptor
			uploadDesc.myData = uniforms[i].data();
			ubo.UploadData(uploadDesc);
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
		}
	}
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