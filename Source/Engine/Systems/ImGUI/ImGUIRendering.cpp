#include "Precomp.h"
#include "ImGUIRendering.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUTexture.h>
#include <Graphics/Resources/UniformBuffer.h>

#include <Core/Profiler.h>

#include "Graphics/NamedFrameBuffers.h"
#include "Graphics/RenderPasses/DebugRenderPass.h"

void ImGUIAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const ImGUIData& data = static_cast<const ImGUIData&>(aData);
	aUB.SetUniform(0, 0, data.myOrthoProj);
}

ImGUIRenderPass::ImGUIRenderPass(Handle<Pipeline> aPipeline, Handle<Texture> aFontAtlas, Graphics& aGraphics)
	: myDestFrameBuffer(DefaultFrameBuffer::kName)
{
	AddDependency(DebugRenderPass::kId);

	aPipeline->ExecLambdaOnLoad([&](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		ASSERT_STR(pipeline->GetAdapterCount() == 1,
			"Only supporting 1 adapter! Please update if changed!");
		const Descriptor& descriptor = pipeline->GetAdapter(0).GetDescriptor();
		myUniformBuffer = aGraphics.CreateUniformBuffer(descriptor.GetBlockSize());
	});
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	Handle<Model> model = new Model(
		Model::PrimitiveType::Triangles, 
		std::span<ImGUIVertex>{},
		true
	);
	myModel = aGraphics.GetOrCreate(model, true, GPUResource::UsageType::Dynamic).Get<GPUModel>();
	myFontAtlas = aGraphics.GetOrCreate(aFontAtlas).Get<GPUTexture>();
}

void ImGUIRenderPass::ScheduleFrame(ImGUIFrame&& aFrame)
{
	myFrames.GetWrite() = std::move(aFrame);
	myFrames.AdvanceWrite();
}

bool ImGUIRenderPass::IsReady() const
{
	return myPipeline->GetState() == GPUResource::State::Valid
		&& myFontAtlas->GetState() == GPUResource::State::Valid
		&& (myModel->GetState() == GPUResource::State::Valid
			|| myModel->GetState() == GPUResource::State::PendingUpload)
		&& myUniformBuffer->GetState() == GPUResource::State::Valid;
}

void ImGUIRenderPass::OnPrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = myDestFrameBuffer;

	aContext.myEnableBlending = true;
	aContext.myBlendingEq = RenderContext::BlendingEq::Add;
	aContext.mySourceBlending = RenderContext::Blending::SourceAlpha;
	aContext.myDestinationBlending = RenderContext::Blending::OneMinusSourceAlpha;
	aContext.myScissorMode = RenderContext::ScissorMode::PerObject;
	aContext.myEnableCulling = false;
	aContext.myEnableDepthTest = false;
	aContext.myShouldClearColor = false;
	aContext.myShouldClearDepth = false;
	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());
	aContext.myTexturesToActivate[0] = 0;
}

// We're using BeginPass to generate all work and schedule updates of assets (model)
void ImGUIRenderPass::BeginPass(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("ImGUIRenderPass::BeginPass");

	PrepareContext(aGraphics);

	myCurrentJob = &aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	myCurrentJob->Clear();

	if (!myFrames.CanReadNext())
	{
		return;
	}

	myFrames.AdvanceRead();
	ImGUIFrame& frame = myFrames.GetRead();

	UniformBlock block(*myUniformBuffer.Get(), myPipeline->GetAdapter(0).GetDescriptor());
	block.SetUniform(0, 0, frame.myMatrix);

	for (Params& params : frame.myParams)
	{
		GPUTexture* texture = myFontAtlas.Get();
		if (params.myTexture.IsValid())
		{
			texture = params.myTexture.Get();
		}

		RenderJob& job = myCurrentJob->AllocateJob();
		job.SetPipeline(myPipeline.Get());
		job.SetModel(myModel.Get());
		job.GetTextures().PushBack(texture);
		job.GetUniformSet().PushBack(myUniformBuffer.Get());

		for (uint8_t i = 0; i < 4; i++)
		{
			job.SetScissorRect(i, params.myScissorRect[i]);
		}

		RenderJob::IndexedDrawParams drawParams;
		drawParams.myOffset = params.myOffset;
		drawParams.myCount = params.myCount;
		job.SetDrawParams(drawParams);
	}

	if (frame.myDesc.myIndCount > 0)
	{
		myModel->GetResource().Get<Model>()->Update(frame.myDesc);
		myModel->UpdateRegion({});
	}
}