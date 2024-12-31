#pragma once

#include <Graphics/GPUResource.h>

// TODO: add support fo dynamic count of per-frame buffers for SSBO
class GPUBuffer : public GPUResource
{
public:
	GPUBuffer(size_t anAlignedSize, uint8_t aFrameCount)
		: myBufferSize(anAlignedSize)
		, myFrameCount(aFrameCount)
	{
		myWriteHead = 1;
		myReadHead = 0;
	}

	char* Map()
	{
		return static_cast<char*>(myMappedBuffer) + myBufferSize * myWriteHead;
	}

	void Unmap()
	{
		myWriteHead = (myWriteHead + 1) % myFrameCount;
		myCanAdvance = true;
	}

	void AdvanceReadBuffer()
	{
		// TODO: I don't like this, but don't have a better idea for now :/
		// In theory I should be able to advance it on bind, but it asserts
		if (myCanAdvance) 
		{
			myReadHead = (myReadHead + 1) % myFrameCount;
		}
		myCanAdvance = false;
	}

	std::string_view GetTypeName() const final { return "GPUBuffer"; }

protected:
	void CheckOverlap() const
	{
		ASSERT_STR(myFrameCount == 1 || myReadHead != myWriteHead, "Read caught up with write!");
	}

	void* myMappedBuffer = nullptr;
	size_t myBufferSize;
	uint8_t myFrameCount; // or how many per-frame sub-buffers it has
	bool myCanAdvance = false;

	// TODO: explore falshe-sharing impact
	std::atomic_uint8_t myWriteHead;
	std::atomic_uint8_t myReadHead;
};