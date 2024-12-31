#include "Precomp.h"
#include "RenderPass.h"

#include <Core/Profiler.h>
#include <Core/CmdBuffer.h>

#include "Descriptor.h"
#include "Graphics.h"
#include "Resources/GPUBuffer.h"
#include "Resources/GPUPipeline.h"
#include "RenderPassJob.h"

GPUBuffer* RenderPass::AllocateUBO(Graphics& aGraphics, size_t aSize)
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

bool RenderPass::BindUBOs(CmdBuffer& aCmdBuffer, Graphics& aGraphics,
	const AdapterSourceData& aSource, const GPUPipeline& aPipeline)
{
	const size_t uboCount = aPipeline.GetAdapterCount();
	constexpr uint8_t kBufferCount = 16;
	ASSERT_STR(uboCount < kBufferCount, "Need more than {} GPUBuffers!", kBufferCount);
	std::array<GPUBuffer*, kBufferCount> buffers;
	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = aPipeline.GetAdapter(i);
		GPUBuffer* uniformBuffer = AllocateUBO(
			aGraphics,
			uniformAdapter.GetDescriptor().GetBlockSize()
		);
		if (!uniformBuffer)
		{
			return false;
		}
		buffers[i] = uniformBuffer;
	}

	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = aPipeline.GetAdapter(i);
		UniformBlock uniformBlock(*buffers[i]);
		uniformAdapter.Fill(aSource, uniformBlock);
		
		RenderPassJob::SetBufferCmd& cmd = aCmdBuffer.Write<RenderPassJob::SetBufferCmd, false>();
		cmd.myBuffer = buffers[i];
		cmd.mySlot = uniformAdapter.GetBindpoint();
		cmd.myType = RenderPassJob::GPUBufferType::Uniform;
	}
	return true;
}

bool RenderPass::BindUBOsAndGrow(CmdBuffer& aCmdBuffer, Graphics& aGraphics,
	const AdapterSourceData& aSource, const GPUPipeline& aPipeline)
{
	const size_t uboCount = aPipeline.GetAdapterCount();
	constexpr uint8_t kBufferCount = 16;
	ASSERT_STR(uboCount < kBufferCount, "Need more than {} GPUBuffers!", kBufferCount);
	std::array<GPUBuffer*, kBufferCount> buffers;
	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = aPipeline.GetAdapter(i);
		GPUBuffer* uniformBuffer = AllocateUBO(
			aGraphics,
			uniformAdapter.GetDescriptor().GetBlockSize()
		);
		if (!uniformBuffer)
		{
			return false;
		}
		buffers[i] = uniformBuffer;
	}

	for (size_t i = 0; i < uboCount; i++)
	{
		const UniformAdapter& uniformAdapter = aPipeline.GetAdapter(i);
		UniformBlock uniformBlock(*buffers[i]);
		uniformAdapter.Fill(aSource, uniformBlock);

		RenderPassJob::SetBufferCmd& cmd = aCmdBuffer.Write<RenderPassJob::SetBufferCmd>();
		cmd.myBuffer = buffers[i];
		cmd.mySlot = uniformAdapter.GetBindpoint();
		cmd.myType = RenderPassJob::GPUBufferType::Uniform;
	}
	return true;
}

size_t RenderPass::GetUBOCount() const
{
	size_t count = 0;
	for (const UBOBucket& bucket : myUBOBuckets)
	{
		count += bucket.myUBOs.size();
	}
	return count;
}

size_t RenderPass::GetUBOTotalSize() const
{
	size_t totalSize = 0;
	for (const UBOBucket& bucket : myUBOBuckets)
	{
		totalSize += GraphicsConfig::kMaxFramesScheduled * 
			bucket.mySize * bucket.myUBOs.size();
	}
	return totalSize;
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

GPUBuffer* RenderPass::UBOBucket::AllocateUBO(Graphics& aGraphics, size_t aSize)
{
	uint32_t index = myUBOCounter++;
	if (index < myUBOs.size())
	{
		Handle<GPUBuffer>& uboHandle = myUBOs[index];
		GPUBuffer* ubo = uboHandle.Get();
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
			myUBOs[i] = aGraphics.CreateGPUBuffer(mySize, GraphicsConfig::kMaxFramesScheduled + 1, true);
		}
	}
	myUBOCounter = 0;
}