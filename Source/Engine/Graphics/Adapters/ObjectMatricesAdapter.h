#pragma once

#include "UniformAdapter.h"

// This basic adapter only provides Model and MVP matrices as uniforms.
// To provide different versions of the adapter, just inherit the adapter
// and override the FillUniformBlock.
class ObjectMatricesAdapter : public UniformAdapter
{
	DECLARE_REGISTER(ObjectMatricesAdapter);
public:
	// Both aGO and aVO can be used to access information needed to render the object
	ObjectMatricesAdapter(const GameObject& aGO, const VisualObject& aVO);

	void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const override;
};