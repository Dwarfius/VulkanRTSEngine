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
	myDependencies.push_back(DebugRenderPass::kId);

	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();

	struct PosUVVertex
	{
		glm::vec2 myPos;
		glm::vec2 myUV;

		PosUVVertex() = default;
		constexpr PosUVVertex(glm::vec2 aPos, glm::vec2 aUV)
			: myPos(aPos)
			, myUV(aUV)
		{
		}

		static constexpr VertexDescriptor GetDescriptor()
		{
			using ThisType = PosUVVertex; // for copy-paste convenience
			return {
				sizeof(ThisType),
				2,
				{
					{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myPos) },
					{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myUV) }
				}
			};
		}
	};

	PosUVVertex vertices[] = {
		{ { -1.f,  1.f }, { 0.f, 1.f } },
		{ { -1.f, -1.f }, { 0.f, 0.f } },
		{ {  1.f, -1.f }, { 1.f, 0.f } },

		{ { -1.f,  1.f }, { 0.f, 1.f } },
		{ {  1.f, -1.f }, { 1.f, 0.f } },
		{ {  1.f,  1.f }, { 1.f, 1.f } }
	};
	Model::VertStorage<PosUVVertex>* buffer = new Model::VertStorage<PosUVVertex>(6, vertices);
	Handle<Model> cpuModel = new Model(Model::PrimitiveType::Triangles, buffer, false);
	myFrameQuadModel = aGraphics.GetOrCreate(cpuModel).Get<GPUModel>();
}

void FinalCompositeRenderPass::SubmitJobs(Graphics& aGraphics)
{
	if (myPipeline->GetState() != GPUResource::State::Valid
		|| myFrameQuadModel->GetState() != GPUResource::State::Valid)
	{
		return;
	}

	RenderPassJob& passJob = aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();

	RenderJob job(myPipeline, myFrameQuadModel, {});
	RenderJob::ArrayDrawParams params;
	params.myOffset = 0;
	params.myCount = 6;
	job.SetDrawParams(params);

	passJob.Add(job);
}

void FinalCompositeRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myFrameBuffer = "";
	aContext.myFrameBufferReadTextures[0] = {
		DefaultFrameBuffer::kName,
		DefaultFrameBuffer::kColorInd,
		RenderContext::FrameBufferTexture::Type::Color
	};

	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());
}