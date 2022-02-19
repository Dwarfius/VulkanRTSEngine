#pragma once

#include <Graphics/UniformAdapterRegister.h>

class SkeletonAdapter : RegisterUniformAdapter<SkeletonAdapter>
{
public:
	constexpr static std::string_view kName = "SkeletonAdapter";
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};