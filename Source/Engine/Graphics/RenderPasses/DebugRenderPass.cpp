#include "Precomp.h"
#include "DebugRenderPass.h"

#include "Graphics/RenderPasses/GenericRenderPasses.h"
#include "Graphics/NamedFrameBuffers.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/UniformBlock.h>

#include <Core/Debug/DebugDrawer.h>

DebugRenderPass::DebugRenderPass(Graphics& aGraphics, Handle<Pipeline> aPipeline)
{
	myPipeline = aGraphics.GetOrCreate(aPipeline).Get<GPUPipeline>();

	myDependencies.push_back(DefaultRenderPass::kId);
}

DebugRenderPass::~DebugRenderPass()
{
	for (PerCameraModel& model : myCameraModels)
	{
		constexpr uint32_t kMaxExtraDescriptors = 32;
		PerCameraModel::UploadDesc* descriptors[kMaxExtraDescriptors];
		uint32_t descInd = 0;
		for (PerCameraModel::UploadDesc* currDesc = model.myDesc.myNextDesc;
			currDesc != nullptr;
			currDesc = currDesc->myNextDesc)
		{
			ASSERT_STR(descInd < kMaxExtraDescriptors, "Descriptor buffer not big enough!");
			descriptors[descInd++] = currDesc;
		}
		while (descInd > 0)
		{
			delete descriptors[--descInd];
		}
	}
}

bool DebugRenderPass::IsReady() const
{
	return myPipeline->GetState() == GPUResource::State::Valid;
}

void DebugRenderPass::SetCamera(uint32_t aCamIndex, const Camera& aCamera, Graphics& aGraphics)
{
	if (aCamIndex >= myCameraModels.size())
	{
		Model::BaseStorage* storage = new Model::VertStorage<PosColorVertex>(0);
		Handle<Model> model = new Model(Model::PrimitiveType::Lines, storage, false);
		Handle<GPUModel> gpuModel = aGraphics.GetOrCreate(model, 
			true, 
			GPUResource::UsageType::Dynamic
		).Get<GPUModel>();
		
		PerCameraModel::UploadDesc desc;
		std::memset(&desc, 0, sizeof(PerCameraModel::UploadDesc));

		ASSERT_STR(myPipeline->GetState() == GPUResource::State::Valid,
			"Not ready to add cameras, pipeline hasn't loaded yet!");
		ASSERT_STR(myPipeline->GetAdapterCount() == 1, 
			"DebugRenderPass needs a pipeline with Camera adapter only!");
		const size_t bufferSize = myPipeline->GetAdapter(0).GetDescriptor().GetBlockSize();
		Handle<UniformBuffer> buffer = aGraphics.CreateUniformBuffer(bufferSize);
		myCameraModels.emplace_back(gpuModel, aCamera, desc, buffer);
	}
	else
	{
		myCameraModels[aCamIndex].myCamera = aCamera;
	}
}

void DebugRenderPass::AddDebugDrawer(uint32_t aCamIndex, const DebugDrawer& aDebugDrawer)
{
	PerCameraModel& perCamModel = myCameraModels[aCamIndex];

	// there's always 1 space allocated for debug drawer, but we might need more
	// if there are more drawers. Look for a slot that's free (doesn't have vertices)
	// or create one if there isn't one.
	PerCameraModel::UploadDesc* currDesc;
	for (currDesc = &perCamModel.myDesc;
		currDesc->myVertCount != 0; // keep iterating until we find an empty one
		currDesc = currDesc->myNextDesc)
	{
		if (currDesc->myNextDesc == nullptr)
		{
			currDesc->myNextDesc = new PerCameraModel::UploadDesc();
			std::memset(currDesc->myNextDesc, 0, sizeof(PerCameraModel::UploadDesc));
		}
	}

	// we've found a free one - fill it up
	currDesc->myVertices = aDebugDrawer.GetCurrentVertices();
	currDesc->myVertCount = aDebugDrawer.GetCurrentVertexCount();
}

void DebugRenderPass::PrepareContext(RenderContext& aContext, Graphics& aGraphics) const
{
	aContext.myFrameBuffer = DefaultFrameBuffer::kName;

	aContext.myScissorMode = RenderContext::ScissorMode::None;
	aContext.myViewportSize[0] = static_cast<int>(aGraphics.GetWidth());
	aContext.myViewportSize[1] = static_cast<int>(aGraphics.GetHeight());
}

void DebugRenderPass::BeginPass(Graphics& anInterface)
{
	IRenderPass::BeginPass(anInterface);

	for (PerCameraModel& perCamModel : myCameraModels)
	{
		// clean up the upload descriptors from the previous frame
		for (PerCameraModel::UploadDesc* currDesc = &perCamModel.myDesc;
			currDesc != nullptr;
			currDesc = currDesc->myNextDesc)
		{
			currDesc->myVertCount = 0;
		}
	}
}

void DebugRenderPass::SubmitJobs(Graphics& anInterface)
{
	if (!IsReady())
	{
		return;
	}

	ASSERT_STR(myPipeline->GetAdapterCount() == 1,
		"DebugRenderPass needs a pipeline with Camera adapter only!");

	RenderPassJob& passJob = anInterface.GetRenderPassJob(GetId(), myRenderContext);
	passJob.Clear();

	const UniformAdapter& adapter = myPipeline->GetAdapter(0);
	for (PerCameraModel& perCamModel : myCameraModels)
	{
		const bool hasDebugData = perCamModel.myDesc.myVertCount > 0;
		if (!hasDebugData || perCamModel.myBuffer->GetState() != GPUResource::State::Valid)
		{
			continue;
		}

		// upload the entire chain
		Model* model = perCamModel.myModel->GetResource().Get<Model>();
		model->Update(perCamModel.myDesc);

		// schedule an update on the GPU
		perCamModel.myModel->UpdateRegion({ 0, 0 });

		// Generate job
		RenderJob& job = passJob.AllocateJob();
		job.SetPipeline(myPipeline.Get());
		job.SetModel(perCamModel.myModel.Get());

		RenderJob::ArrayDrawParams params;
		params.myOffset = 0;
		params.myCount = static_cast<uint32_t>(model->GetVertexCount());
		job.SetDrawParams(params);

		AdapterSourceData source{ anInterface, perCamModel.myCamera };

		UniformBlock block(*perCamModel.myBuffer.Get(), adapter.GetDescriptor());
		adapter.Fill(source, block);
		job.GetUniformSet().PushBack(perCamModel.myBuffer.Get());
	}
}