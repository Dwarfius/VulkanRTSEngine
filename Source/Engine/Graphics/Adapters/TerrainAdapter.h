#pragma once

#include "UniformAdapter.h"

class TerrainAdapter : public UniformAdapter
{
	DECLARE_REGISTER(TerrainAdapter);
public:
	// Both aGO and aVO can be used to access information needed to render the object
	TerrainAdapter(const GameObject& aGO, const VisualObject& aVO);

	void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const override;
};