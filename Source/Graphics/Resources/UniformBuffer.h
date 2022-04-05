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

	virtual char* Map() = 0;
	virtual void Unmap() = 0;

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
};