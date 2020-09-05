#pragma once

#include <Graphics/UniformAdapter.h>

class CameraAdapter : public UniformAdapter
{
	DECLARE_REGISTER(CameraAdapter);
public:
	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const override;
};