#include "Precomp.h"
#include "RenderPassJob.h"

#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUTexture.h>

void RenderJob::AddUniformBlock(const Handle<UniformBuffer>& aBuffer)
{
	myUniforms.PushBack(aBuffer);
}

bool RenderJob::HasLastHandles() const
{
	constexpr auto CheckLastHandle = [](const Handle<GPUResource>& aRes) {
		return aRes.IsValid() && aRes.IsLastHandle();
	};

	return CheckLastHandle(myPipeline)
		|| CheckLastHandle(myModel)
		|| std::any_of(myTextures.begin(), myTextures.end(), CheckLastHandle);
}

void RenderJob::SetDrawParams(const IndexedDrawParams& aParams) 
{
	myDrawMode = DrawMode::Indexed;
	myDrawParams.myIndexedParams = aParams; 
}

void RenderJob::SetDrawParams(const TesselationDrawParams& aParams)
{
	myDrawMode = DrawMode::Tesselated;
	myDrawParams.myTessParams = aParams;
}

void RenderJob::SetDrawParams(const ArrayDrawParams& aParams)
{
	myDrawMode = DrawMode::Array;
	myDrawParams.myArrayParams = aParams;
}

//================================================

RenderJob& RenderPassJob::AllocateJob()
{
	tbb::spin_mutex::scoped_lock lock(myJobsMutex);
	return myJobs.Allocate();
}

void RenderPassJob::Execute(Graphics& aGraphics)
{
	BindFrameBuffer(aGraphics, myContext);
	Clear(myContext);

	if (!myJobs.IsEmpty())
	{
		SetupContext(aGraphics, myContext);
		RunJobs(myJobs);
	}
}

void RenderPassJob::Initialize(const RenderContext& aContext)
{
	myContext = aContext;
	OnInitialize(myContext);
}