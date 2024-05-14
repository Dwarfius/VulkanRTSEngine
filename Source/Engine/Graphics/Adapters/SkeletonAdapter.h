#pragma once

#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>

class SkeletonAdapter : RegisterUniformAdapter<SkeletonAdapter>
{
public:
	constexpr static uint32_t kMaxBones = 100;
	constexpr static Descriptor ourDescriptor{
		{ Descriptor::UniformType::Mat4, kMaxBones }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};