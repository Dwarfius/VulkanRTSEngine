#include "Precomp.h"
#include "RenderPass.h"

#include "RenderContext.h"
#include "Graphics.h"
#include "Resources/UniformBuffer.h"

RenderJob& RenderPass::AllocateJob()
{
	return myCurrentJob->AllocateJob();
}

UniformBuffer* RenderPass::AllocateUBO(Graphics& aGraphics, size_t aSize)
{
	auto bucketIter = std::lower_bound(
		myUBOBuckets.begin(), 
		myUBOBuckets.end(), 
		aSize
	);
	if (bucketIter != myUBOBuckets.end() && bucketIter->mySize == aSize)
	{
		return bucketIter->AllocateUBO(aGraphics, aSize);
	}
	myNewBuckets.emplace(aSize);
	return nullptr;
}

void RenderPass::BeginPass(Graphics& aGraphics)
{
	PrepareContext(aGraphics);

	myCurrentJob = &aGraphics.GetRenderPassJob(GetId(), myRenderContext);
	myCurrentJob->Clear();
	// Note: this is not thread safe if same pass is started concurrently
	for (size_t newBucket : myNewBuckets)
	{
		myUBOBuckets.emplace_back(newBucket);
	}
	myNewBuckets.clear();

	for (UBOBucket& uboBucket : myUBOBuckets)
	{
		uboBucket.PrepForPass(aGraphics);
	}
	std::sort(myUBOBuckets.begin(), myUBOBuckets.end());
}

void RenderPass::PrepareContext(Graphics& aGraphics)
{
	if (!myHasValidContext || HasDynamicRenderContext())
	{
		OnPrepareContext(myRenderContext, aGraphics);
		myHasValidContext = true;
	}
}

void RenderPass::PreallocateUBOs(size_t aSize)
{
	myNewBuckets.emplace(aSize);
}

RenderPass::UBOBucket::UBOBucket(size_t aSize)
	: mySize(aSize)
{
}

UniformBuffer* RenderPass::UBOBucket::AllocateUBO(Graphics& aGraphics, size_t aSize)
{
	uint32_t index = myUBOCounter++;
	if (index < myUBOs.size())
	{
		Handle<UniformBuffer>& uboHandle = myUBOs[index];
		UniformBuffer* ubo = uboHandle.Get();
		return ubo->GetState() == GPUResource::State::Valid ?
			ubo : nullptr;
	}
	return nullptr;
}

void RenderPass::UBOBucket::PrepForPass(Graphics& aGraphics)
{
	if (myUBOCounter >= myUBOs.size())
	{
		const float counter = static_cast<float>(myUBOCounter);
		const float nextLog2 = glm::floor(glm::log2(counter) + 1);
		const size_t newSize = static_cast<size_t>(glm::exp2(nextLog2));
		const size_t oldSize = myUBOs.size();
		myUBOs.resize(newSize);
		for (size_t i=oldSize; i < newSize; i++)
		{
			myUBOs[i] = aGraphics.CreateUniformBuffer(mySize);
		}
	}
	myUBOCounter = 0;
}