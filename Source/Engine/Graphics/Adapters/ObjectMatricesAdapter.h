#pragma once

#include <Graphics/UniformAdapterRegister.h>

// This basic adapter only provides Model and MVP matrices as uniforms.
// To provide different versions of the adapter, just inherit the adapter
// and override the FillUniformBlock.
class ObjectMatricesAdapter : RegisterUniformAdapter<ObjectMatricesAdapter>
{
public:
	constexpr static std::string_view kName = "ObjectMatricesAdapter";
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};