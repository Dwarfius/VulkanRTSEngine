#pragma once

#include "../GPUResource.h"
#include <Core/RWBuffer.h>

class UniformBuffer : public GPUResource
{
public:
	UniformBuffer(size_t anAlignedSize)
		: myBufferSize(anAlignedSize)
	{
	}

	char* Map()
	{
		const Buffer& buffer = myBufferInfos.GetWrite();
		return static_cast<char*>(myMappedBuffer) + buffer.myOffest;
	}

	void Unmap()
	{
		myBufferInfos.AdvanceWrite();
		myWasMapped = true;
	}

	void AdvanceReadBuffer()
	{
		if (myWasMapped)
		{
			myBufferInfos.AdvanceRead();
			myWasMapped = false;
		}
	}

protected:
	// TODO: move this to a compile time constants struct
	static constexpr uint8_t kMaxFrames = 3;

	struct Buffer
	{
		size_t myOffest = 0;
	};
	RWBuffer<Buffer, kMaxFrames> myBufferInfos;
	void* myMappedBuffer = nullptr;
	const size_t myBufferSize;
	bool myWasMapped = false;
};