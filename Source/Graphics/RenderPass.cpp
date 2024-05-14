#include "Precomp.h"
#include "RenderPass.h"

#include "Descriptor.h"
#include "Graphics.h"
#include "Resources/UniformBuffer.h"
#include "Resources/GPUPipeline.h"

#include <Core/Profiler.h>

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

bool RenderPass::FillUBOs(RenderJob::UniformSet& aSet, Graphics& aGraphics, 
	const AdapterSourceData& aSource, const GPUPipeline& aPipeline)
{
	const size_t uboCount = aPipeline.GetAdapterCount();
	ASSERT_STR(uboCount <= 4,
		"Tried to push {} UBOs into a render job that supports only 4!",
		uboCount);

	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = aPipeline.GetAdapter(i);
		if (uniformAdapter.IsGlobal())
		{
			aSet.PushBack(uniformAdapter.GetGlobalUBO().Get());
			continue;
		}

		UniformBuffer* uniformBuffer = AllocateUBO(
			aGraphics,
			uniformAdapter.GetDescriptor().GetBlockSize()
		);
		if (!uniformBuffer)
		{
			return false;
		}

		UniformBlock uniformBlock(*uniformBuffer);
		uniformAdapter.Fill(aSource, uniformBlock);
		aSet.PushBack(uniformBuffer);
	}
	return true;
}

void RenderPass::Execute(Graphics& aGraphics)
{
	Profiler::ScopedMark mark("RenderPass::Execute");
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