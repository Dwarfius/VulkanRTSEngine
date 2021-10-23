#include "Precomp.h"
#include "RenderPassJob.h"

#include <Graphics/UniformBlock.h>
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

void RenderJob::SetUniformSet(const std::vector<std::shared_ptr<UniformBlock>>& aUniformSet)
{
	for (const std::shared_ptr<UniformBlock>& block : aUniformSet)
	{
		AddUniformBlock(*block);
	}
}

void RenderJob::AddUniformBlock(const UniformBlock& aBlock)
{
	// TODO: replace with a caching allocator, cause this is allocation
	// per block per job per frame (which is a lot!)
	std::vector<char> buffer;
	buffer.resize(aBlock.GetSize());
	std::memcpy(buffer.data(), aBlock.GetData(), aBlock.GetSize());
	myUniforms.push_back(std::move(buffer));
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