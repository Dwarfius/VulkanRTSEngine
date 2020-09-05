#pragma once

#include <Graphics/UniformAdapter.h>

// This basic adapter only provides Model and MVP matrices as uniforms.
// To provide different versions of the adapter, just inherit the adapter
// and override the FillUniformBlock.
class ObjectMatricesAdapter : public UniformAdapter
{
	DECLARE_REGISTER(ObjectMatricesAdapter);
public:
	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const override;
};