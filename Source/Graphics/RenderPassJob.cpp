#include "Precomp.h"
#include "RenderPassJob.h"

#include <Graphics/Resources/UniformBuffer.h>
#include <Graphics/Resources/GPUPipeline.h>
#include <Graphics/Resources/GPUModel.h>
#include <Graphics/Resources/GPUTexture.h>

RenderJob::RenderJob(Handle<GPUPipeline> aPipeline, Handle<GPUModel> aModel,
	const TextureSet& aTextures)
	: myPipeline(aPipeline)
	, myModel(aModel)
	, myTextures(aTextures)
	, myScissorRect{ 0,0,0,0 }
	, myDrawMode(DrawMode::Indexed)
	, myDrawParams()
{
}

void RenderJob::SetUniformSet(const UniformSet& aUniformSet)
{
	for (const Handle<UniformBuffer>& buffer : aUniformSet)
	{
		AddUniformBlock(buffer);
	}
}

void RenderJob::AddUniformBlock(const Handle<UniformBuffer>& aBuffer)
{
	myUniforms.push_back(aBuffer);
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

void RenderPassJob::Execute(Graphics& aGraphics)
{
	BindFrameBuffer(aGraphics, myContext);
	Clear(myContext);

	if (HasWork())
	{
		SetupContext(aGraphics, myContext);
		RunJobs();
	}
}

void RenderPassJob::Initialize(const RenderContext& aContext)
{
	myContext = aContext;
	OnInitialize(myContext);
}