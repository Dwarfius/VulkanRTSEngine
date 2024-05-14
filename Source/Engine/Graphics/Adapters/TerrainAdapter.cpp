#include "Precomp.h"
#include "TerrainAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../GameObject.h"
#include "../../Terrain.h"
#include "../../Game.h"
#include "AdapterSourceData.h"
#include "VisualObject.h"

float TerrainAdapter::GetTileSize(const Terrain& aTerrain)
{
	const glm::vec3 size = glm::vec3(aTerrain.GetWidth(), 0, aTerrain.GetDepth());

	ASSERT_STR(size.x == size.z, "Non-square terrain isn't implemented yet!");

	return glm::min(kTileSize, size.x);
}

glm::ivec2 TerrainAdapter::GetTileCount(const Terrain& aTerrain)
{
	const float tileSize = GetTileSize(aTerrain);
	const glm::vec3 size = glm::vec3(aTerrain.GetWidth(), 0, aTerrain.GetDepth());
	const glm::vec3 tiles = glm::ceil(size / tileSize);

	const int gridWidth = static_cast<int>(tiles.x);
	const int gridDepth = static_cast<int>(tiles.z);
	return glm::ivec2(gridWidth, gridDepth);
}

void TerrainAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const Source& source = static_cast<const Source&>(aData);

	const Terrain& terrain = source.myTerrain;

	glm::vec3 terrainOrigin = source.myVO.GetTransform().GetPos();
	// move origin to center
	terrainOrigin -= glm::vec3(terrain.GetWidth(), 0, terrain.GetDepth()) / 2.f;
	aUB.SetUniform(ourDescriptor.GetOffset(0, 0), terrainOrigin);
	aUB.SetUniform(ourDescriptor.GetOffset(1, 0), GetTileSize(terrain)); // TODO: split into XY size instead of X size

	const glm::ivec2 gridTiles = GetTileCount(terrain);
	aUB.SetUniform(ourDescriptor.GetOffset(2, 0), gridTiles.x);
	aUB.SetUniform(ourDescriptor.GetOffset(3, 0), gridTiles.y);
	aUB.SetUniform(ourDescriptor.GetOffset(4, 0), terrain.GetYScale());

	static_assert(kHeightLayers == Terrain::kMaxHeightLevels,
		"Please ensure these constants are the same!");
	const uint8_t layersCount = terrain.GetHeightLevelCount();
	for (uint8_t i = 0; i < layersCount; i++)
	{
		const Terrain::HeightLevelColor heightLevel = terrain.GetHeightLevelColor(i);
		aUB.SetUniform(ourDescriptor.GetOffset(5, i), glm::vec4{ heightLevel.myColor, heightLevel.myHeight });
	}
	const Terrain::HeightLevelColor heightLevel = terrain.GetHeightLevelColor(layersCount - 1);
	for (uint8_t i = layersCount; i < kHeightLayers; i++)
	{
		aUB.SetUniform(ourDescriptor.GetOffset(5, i), glm::vec4{ heightLevel.myColor, heightLevel.myHeight });
	}
}
