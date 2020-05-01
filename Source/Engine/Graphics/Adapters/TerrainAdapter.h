#pragma once

#include "UniformAdapter.h"

class Terrain;

class TerrainAdapter : public UniformAdapter
{
	DECLARE_REGISTER(TerrainAdapter);

	constexpr static float kTileSize = 64.f;

	static float GetTileSize(const Terrain* aTerrain);

public:
	// Returns in how many tiles is the terrain grid split
	static glm::ivec2 GetTileCount(const Terrain* aTerrain);

	// Both aGO and aVO can be used to access information needed to render the object
	TerrainAdapter(const GameObject& aGO, const VisualObject& aVO);

	void FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const override;
};