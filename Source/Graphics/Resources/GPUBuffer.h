#pragma once

#include <Core/RWBuffer.h>

#include <Graphics/GraphicsConfig.h>
#include <Graphics/GPUResource.h>

// TODO: add support fo dynamic count of per-frame buffers for SSBO
class GPUBuffer : public GPUResource
{
public:
	enum class Type : uint8_t
	{
		Uniform,
		ShaderStorage
	};

	GPUBuffer(Type aType, size_t anAlignedSize)
		: myBufferSize(anAlignedSize)
		, myType(aType)
	{
	}

	char* Map()
	{
		if (myType == Type::Uniform)
		{
			const Buffer& buffer = myBufferInfos.GetWrite();
			return static_cast<char*>(myMappedBuffer) + buffer.myOffest;
		}
		return static_cast<char*>(myMappedBuffer);
	}

	void Unmap()
	{
		if (myType == Type::Uniform)
		{
			myBufferInfos.AdvanceWrite();
			myMappedCount++;
		}
	}

	void AdvanceReadBuffer()
	{
		if (myMappedCount)
		{
			myBufferInfos.AdvanceRead();
			myMappedCount--;
		}
	}

	Type GetType() const { return myType; }

	std::string_view GetTypeName() const final { return "GPUBuffer"; }

protected:
	static constexpr uint8_t kMaxFrames = GraphicsConfig::kMaxFramesScheduled + 1;

	struct Buffer
	{
		size_t myOffest = 0;
	};
	RWBuffer<Buffer, kMaxFrames> myBufferInfos;
	void* myMappedBuffer = nullptr;
	size_t myBufferSize;
	uint8_t myMappedCount = 0;
	Type myType;
};