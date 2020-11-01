#pragma once

#include <Graphics/UniformAdapter.h>

class SkeletonAdapter : public UniformAdapter
{
	DECLARE_REGISTER(SkeletonAdapter);
public:
	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const override;
};