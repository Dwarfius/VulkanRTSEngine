#pragma once

#include "UniformAdapter.h"

class CameraAdapter : public UniformAdapter
{
	DECLARE_REGISTER(CameraAdapter);
public:
	// Both aGO and aVO can be used to access information needed to render the object
	CameraAdapter(const GameObject& aGO, const VisualObject& aVO);

	void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const override;
};