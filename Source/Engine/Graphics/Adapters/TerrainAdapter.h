#pragma once

#include <Graphics/UniformAdapter.h>
#include "Graphics/Adapters/AdapterSourceData.h"

class Terrain;

class TerrainAdapter : public UniformAdapter
{
	DECLARE_REGISTER(TerrainAdapter);

	constexpr static float kTileSize = 64.f;

	static float GetTileSize(const Terrain& aTerrain);

public:
	struct Source : UniformAdapterSource
	{
		const Terrain& myTerrain;
	};

	// Returns in how many tiles is the terrain grid split
	static glm::ivec2 GetTileCount(const Terrain& aTerrain);

	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const override;
};