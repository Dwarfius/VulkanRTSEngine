#pragma once

#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Descriptor.h>
#include "AdapterSourceData.h"

class Terrain;

class TerrainAdapter : RegisterUniformAdapter<TerrainAdapter>
{
	constexpr static float kTileSize = 64.f;

	static float GetTileSize(const Terrain& aTerrain);

public:
	struct Source : UniformAdapterSource
	{
		const Terrain& myTerrain;
	};

	// Returns in how many tiles is the terrain grid split
	static glm::ivec2 GetTileCount(const Terrain& aTerrain);

	constexpr static uint8_t kHeightLayers = 5;
	inline static const Descriptor ourDescriptor{
		{ Descriptor::UniformType::Vec4 },
		{ Descriptor::UniformType::Float },
		{ Descriptor::UniformType::Int },
		{ Descriptor::UniformType::Int },
		{ Descriptor::UniformType::Float },
		{ Descriptor::UniformType::Vec4, kHeightLayers }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};