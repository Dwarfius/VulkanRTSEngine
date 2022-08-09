#include "Precomp.h"
#include "RenderPass.h"

#include "RenderContext.h"
#include "Graphics.h"
#include "Resources/UniformBuffer.h"

void IRenderPass::BeginPass(Graphics& anInterface)
{
	if (!myHasValidContext || HasDynamicRenderContext())
	{
		PrepareContext(myRenderContext, anInterface);
		myHasValidContext = true;
	}
}

// ===============================================================

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

void RenderPass::BeginPass(Graphics& anInterface)
{
	IRenderPass::BeginPass(anInterface);

	myCurrentJob = &anInterface.GetRenderPassJob(GetId(), myRenderContext);
	myCurrentJob->Clear();
	// Note: this is not thread safe if same pass is started concurrently
	for (UBOBucket& uboBucket : myUBOBuckets)
	{
		uboBucket.PrepForPass(anInterface);
	}

	for (size_t newBucket : myNewBuckets)
	{
		myUBOBuckets.emplace_back(newBucket);
	}
	myNewBuckets.clear();
	std::sort(myUBOBuckets.begin(), myUBOBuckets.end());
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
	// Just a way to start with a 32-ubo set on first growth
	if (myUBOCounter == 0)
	{
		myUBOCounter = 32;
	}

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