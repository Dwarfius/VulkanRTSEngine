#include "Precomp.h"
#include "RenderPassJob.h"

#include <Graphics/UniformBlock.h>

RenderJob::RenderJob(Handle<GPUResource> aPipeline, Handle<GPUResource> aModel,
	const TextureSet& aTextures)
	: myPipeline(aPipeline)
	, myModel(aModel)
	, myTextures(aTextures)
	, myScissorRect{ 0,0,0,0 }
	, myPriority(0)
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

//================================================

void RenderPassJob::Execute()
{
	if (myContext.myShouldClearColor || myContext.myShouldClearDepth)
	{
		Clear(myContext);
	}

	if (HasWork())
	{
		SetupContext(myContext);
		RunJobs();
	}
}

void RenderPassJob::Initialize(const RenderContext& aContext)
{
	myContext = aContext;
	OnInitialize(myContext);
}