#include "Precomp.h"
#include "RenderPassJobGL.h"

#include "PipelineGL.h"
#include "ModelGL.h"
#include "TextureGL.h"

#include <Graphics/Pipeline.h>
#include <Graphics/UniformBlock.h>
#include <Graphics/Texture.h>

#include "../../Terrain.h"

void RenderPassJobGL::Add(const RenderJob& aJob)
{
	myJobs.push_back(aJob);
}

void RenderPassJobGL::AddRange(std::vector<RenderJob>&& aJobs)
{
	myJobs = std::move(aJobs);
}

RenderPassJobGL::operator std::vector<RenderJob>() &&
{
	return myJobs;
}

bool RenderPassJobGL::HasWork() const
{
	return !myJobs.empty();
}

void RenderPassJobGL::OnInitialize(const RenderContext& aContext)
{
	// do nothing explicitly
}

void RenderPassJobGL::SetupContext(const RenderContext& aContext)
{
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
		uint32_t srcBlend = ConvertBlendMode(aContext.mySourceBlending);
		uint32_t dstBlend = ConvertBlendMode(aContext.myDestinationBlending);
		bool hasConstColor = aContext.mySourceBlending == RenderContext::Blending::ConstantColor
			|| aContext.mySourceBlending == RenderContext::Blending::OneMinusConstantColor
			|| aContext.mySourceBlending == RenderContext::Blending::ConstantAlpha
			|| aContext.mySourceBlending == RenderContext::Blending::OneMinusConstantAlpha
			|| aContext.myDestinationBlending == RenderContext::Blending::ConstantColor
			|| aContext.myDestinationBlending == RenderContext::Blending::OneMinusConstantColor
			|| aContext.myDestinationBlending == RenderContext::Blending::ConstantAlpha
			|| aContext.myDestinationBlending == RenderContext::Blending::OneMinusConstantAlpha;

		glBlendFunc(srcBlend, dstBlend);
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

void RenderPassJobGL::RunJobs()
{
	for (const RenderJob& r : myJobs)
	{
		if (r.HasLastHandles())
		{
			// one of the handles has expired, meaning noone needs it anymore
			// cheaper to skip it
			continue;
		}

		const Pipeline* pipelineRes = r.myPipeline.Get();
		PipelineGL* pipeline = static_cast<PipelineGL*>(pipelineRes->GetGPUResource_Int());
		ModelGL* model = static_cast<ModelGL*>(r.myModel->GetGPUResource_Int());

		if (pipeline != myCurrentPipeline)
		{
			pipeline->Bind();
			myCurrentPipeline = pipeline;

			// binding uniform blocks to according slots
			size_t blockCount = pipeline->GetUBOCount();
			ASSERT_STR(blockCount < numeric_limits<uint32_t>::max(), "Index of UBO block doesn't fit for binding!");
			for (size_t i = 0; i < blockCount; i++)
			{
				// TODO: implement logic that doesn't rebind same slots:
				// If pipeline A has X, Y, Z uniform blocks
				// and pipeline B has X, V, W,
				// if we bind from A to B (or vice versa), no need to rebind
				pipeline->GetUBO(i).Bind(static_cast<uint32_t>(i));
			}
		}

		//if (texture != myCurrentTexture)
		for (size_t textureInd = 0; textureInd < r.myTextures.size(); textureInd++)
		{
			int slotToUse = myTextureSlotsToUse[textureInd];
			if (slotToUse == -1)
			{
				continue;
			}

			TextureGL* texture = static_cast<TextureGL*>(r.myTextures[textureInd]->GetGPUResource_Int());
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
		size_t descriptorCount = pipelineRes->GetDescriptorCount();
		for (size_t i = 0; i < descriptorCount; i++)
		{
			// grabbing the descriptor because it has the locations
			// of uniform values to be uploaded to
			const Descriptor& aDesc = pipelineRes->GetDescriptor(i);
			// there's a UBO for every descriptor of the pipeline
			UniformBufferGL& ubo = pipeline->GetUBO(i);
			UniformBufferGL::UploadDescriptor uploadDesc;
			uploadDesc.mySize = aDesc.GetBlockSize();
			uploadDesc.myData = r.myUniforms[i]->GetData();
			ubo.Upload(uploadDesc);
		}

		switch (r.GetDrawMode())
		{
		case RenderJob::DrawMode::Indexed:
		{
			uint32_t drawMode = model->GetDrawMode();
			size_t primitiveCount = model->GetPrimitiveCount();
			ASSERT_STR(primitiveCount < numeric_limits<GLsizei>::max(), "Exceeded the limit of primitives!");
			glDrawElements(drawMode, static_cast<GLsizei>(primitiveCount), GL_UNSIGNED_INT, 0);
			break;
		}
		case RenderJob::DrawMode::Tesselated:
		{
			// number of control points per patch is hardcoded for now
			// Terrain uses 1 CP to expand it into a quad
			glPatchParameteri(GL_PATCH_VERTICES, 1);

			int instances = r.GetDrawParams().myTessParams.myInstanceCount;
			glDrawArraysInstanced(GL_PATCHES, 0, 1, instances);
			break;
		}
		}
	}
}

uint32_t RenderPassJobGL::ConvertBlendMode(RenderContext::Blending blendMode)
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