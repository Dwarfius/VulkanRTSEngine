#pragma once

#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>

// This basic adapter only provides Model and MVP matrices as uniforms.
// To provide different versions of the adapter, just inherit the adapter
// and override the FillUniformBlock.
class ObjectMatricesAdapter : RegisterUniformAdapter<ObjectMatricesAdapter>
{
public:
	inline static const Descriptor ourDescriptor{
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Mat4 },
		{ Descriptor::UniformType::Mat4 }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};