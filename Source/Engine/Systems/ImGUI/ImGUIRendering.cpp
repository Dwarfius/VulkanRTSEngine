#include "Precomp.h"
#include "ImGUIRendering.h"

#include <Graphics/Graphics.h>
#include <Graphics/GPUResource.h>

void ImGUIAdapter::FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const
{
	const ImGUIData& data = static_cast<const ImGUIData&>(aData);
	aUB.SetUniform(0, 0, data.myOrthoProj);
}

ImGUIRenderPass::ImGUIRenderPass(Handle<Pipeline> aPipeline, Handle<Texture> aFontAtlas, Graphics& aGraphics)
{
	aPipeline->ExecLambdaOnLoad([&](const Resource* aRes) {
		const Pipeline* pipeline = static_cast<const Pipeline*>(aRes);
		ASSERT_STR(pipeline->GetDescriptorCount() == 1,
			"Only supporting 1 descriptor! Please update if changed!");
		const Descriptor& descriptor = pipeline->GetDescriptor(0);
		myUniformBlock = std::make_shared<UniformBlock>(descriptor);
	});
	myPipeline = aGraphics.GetOrCreate(aPipeline);
	Model::BaseStorage* vertexStorage = new Model::VertStorage<ImGUIVertex>(0);
	Handle<Model> model = new Model(Model::PrimitiveType::Triangles, vertexStorage, true);
	// TODO: Extend to pass in the settings of the model to create - this
	// currently creates a STATIC_DRAW buffer, while we need DYNAMIC_DRAW
	myModel = aGraphics.GetOrCreate(model, true);
	myFontAtlas = aGraphics.GetOrCreate(aFontAtlas);
}

void ImGUIRenderPass::SetProj(const glm::mat4& aMatrix)
{
	myUniformBlock->SetUniform(0, 0, aMatrix);
}

void ImGUIRenderPass::UpdateImGuiVerts(const Model::UploadDescriptor<ImGUIVertex>& aDescriptor)
{
	myModel->GetResource().Get<Model>()->Update(aDescriptor);
	myModel->UpdateRegion({});
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
		&& myModel->GetState() == GPUResource::State::Valid;
}

void ImGUIRenderPass::PrepareContext(RenderContext& aContext) const
{
	aContext.myEnableBlending = true;
	aContext.myBlendingEq = RenderContext::BlendingEq::Add;
	aContext.mySourceBlending = RenderContext::Blending::SourceAlpha;
	aContext.myDestinationBlending = RenderContext::Blending::OneMinusSourceAlpha;
	aContext.myScissorMode = RenderContext::ScissorMode::PerObject;
	aContext.myEnableCulling = false;
	aContext.myEnableDepthTest = false;
	aContext.myShouldClearColor = false;
	aContext.myShouldClearDepth = false;
	aContext.myViewportSize[0] = static_cast<int>(Graphics::GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(Graphics::GetHeight());
	aContext.myTexturesToActivate[0] = 0;
}

void ImGUIRenderPass::AddRenderable(RenderJob& aJob, const IParams& aParams)
{
	Process(aJob, aParams);
	myCurrentJob->Add(aJob);
}

// We're using BeginPass to generate all work and schedule updates of assets (model)
void ImGUIRenderPass::BeginPass(Graphics& aGraphics)
{
	AssertLock lock(myRenderJobMutex);
	IRenderPass::BeginPass(aGraphics);

	myCurrentJob = &aGraphics.GetRenderPassJob(Id(), myRenderContext);
	myCurrentJob->Clear();

	for (const ImGUIRenderParams& params : myScheduledImGuiParams)
	{
		RenderJob job{
			myPipeline,
			myModel,
			{ myFontAtlas }
		};
		job.AddUniformBlock(*myUniformBlock);
		AddRenderable(job, params);
	}
	myScheduledImGuiParams.clear();
}

void ImGUIRenderPass::Process(RenderJob& aJob, const IParams& aParams) const
{
	aJob.SetDrawMode(RenderJob::DrawMode::Indexed);
	const ImGUIRenderParams& params = static_cast<const ImGUIRenderParams&>(aParams);
	for (uint8_t i = 0; i < 4; i++)
	{
		aJob.SetScissorRect(i, params.myScissorRect[i]);
	}
	RenderJob::IndexedDrawParams drawParams;
	drawParams.myOffset = params.myOffset;
	drawParams.myCount = params.myCount;
	aJob.SetDrawParams(drawParams);
}