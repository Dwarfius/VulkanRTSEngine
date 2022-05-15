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
	myDependencies.push_back(DebugRenderPass::kId);

	aPipeline->ExecLambdaOnLoad([&](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		ASSERT_STR(pipeline->GetAdapterCount() == 1,
			"Only supporting 1 adapter! Please update if changed!");
		const Descriptor& descriptor = pipeline->GetAdapter(0).GetDescriptor();
		myUniformBuffer = aGraphics.CreateUniformBuffer(descriptor.GetBlockSize());
	});
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();
	Model::BaseStorage* vertexStorage = new Model::VertStorage<ImGUIVertex>(0);
	Handle<Model> model = new Model(Model::PrimitiveType::Triangles, vertexStorage, true);
	myModel = aGraphics.GetOrCreate(model, true, GPUResource::UsageType::Dynamic).Get<GPUModel>();
	myFontAtlas = aGraphics.GetOrCreate(aFontAtlas).Get<GPUTexture>();

	std::memset(&myUpdateDescriptor, 0, sizeof(myUpdateDescriptor));
}

void ImGUIRenderPass::SetProj(const glm::mat4& aMatrix)
{
	UniformBlock block(*myUniformBuffer.Get(), myPipeline->GetAdapter(0).GetDescriptor());
	block.SetUniform(0, 0, aMatrix);
}

void ImGUIRenderPass::UpdateImGuiVerts(const Model::UploadDescriptor<ImGUIVertex>& aDescriptor)
{
	// We can't upload straight to staging model, since it'll 
	// overwrite data pending for the upcoming frame render
	// TODO: fix this once mapped VBOs are in
	myUpdateDescriptor = aDescriptor;
}

void ImGUIRenderPass::AddImGuiRenderJob(const ImGUIRenderParams& aParams)
{
	AssertLock lock(myRenderJobMutex);
	// TODO: I don't like the fact that this has to cache it, this
	// introduces a level of complexity and indirection. To improve this
	// I feel like I need to reorganize how I handle GetRenderPassJob
	myScheduledImGuiParams.push_back(aParams);
}

bool ImGUIRenderPass::IsReady() const
{
	return myPipeline->GetState() == GPUResource::State::Valid
		&& myFontAtlas->GetState() == GPUResource::State::Valid
		&& (myModel->GetState() == GPUResource::State::Valid
			|| myModel->GetState() == GPUResource::State::PendingUpload)
		&& myUniformBuffer->GetState() == GPUResource::State::Valid;
}

void ImGUIRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
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

	AssertLock lock(myRenderJobMutex);
	IRenderPass::BeginPass(aGraphics);

	myCurrentJob = &aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	myCurrentJob->Clear();

	for (ImGUIRenderParams& params : myScheduledImGuiParams)
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

	if (myUpdateDescriptor.myIndCount > 0)
	{
		myModel->GetResource().Get<Model>()->Update(myUpdateDescriptor);
		myModel->UpdateRegion({});
	}

	myScheduledImGuiParams.clear();
}