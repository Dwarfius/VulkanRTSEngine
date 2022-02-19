#pragma once

#include <Graphics/UniformAdapterRegister.h>
#include "Graphics/Adapters/AdapterSourceData.h"

class Terrain;

class TerrainAdapter : RegisterUniformAdapter<TerrainAdapter>
{
	constexpr static float kTileSize = 64.f;

	static float GetTileSize(const Terrain& aTerrain);

public:
	constexpr static std::string_view kName = "TerrainAdapter";

	struct Source : UniformAdapterSource
	{
		const Terrain& myTerrain;
	};

	// Returns in how many tiles is the terrain grid split
	static glm::ivec2 GetTileCount(const Terrain& aTerrain);

	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};