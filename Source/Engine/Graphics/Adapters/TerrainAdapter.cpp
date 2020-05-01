#include "Precomp.h"
#include "TerrainAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../VisualObject.h"
#include "../../Terrain.h"
#include "../../Game.h"

float TerrainAdapter::GetTileSize(const Terrain* aTerrain)
{
	const glm::vec3 size = glm::vec3(aTerrain->GetWidth(), 0, aTerrain->GetDepth());

	ASSERT_STR(size.x == size.z, "Non-square terrain isn't implemented yet!");

	return glm::min(kTileSize, size.x);
}

glm::ivec2 TerrainAdapter::GetTileCount(const Terrain* aTerrain)
{
	const float tileSize = GetTileSize(aTerrain);
	const glm::vec3 size = glm::vec3(aTerrain->GetWidth(), 0, aTerrain->GetDepth());
	const glm::vec3 tiles = glm::ceil(size / tileSize);

	const int gridWidth = static_cast<int>(tiles.x);
	const int gridDepth = static_cast<int>(tiles.z);
	return glm::ivec2(gridWidth, gridDepth);
}

TerrainAdapter::TerrainAdapter(const GameObject& aGO, const VisualObject& aVO)
	: UniformAdapter(aGO, aVO)
{
}

void TerrainAdapter::FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already

	const Terrain* terrain = Game::GetInstance()->GetTerrain(glm::vec3());
	const glm::ivec2 gridTiles = GetTileCount(terrain);

	glm::vec3 terrainOrigin = myVO.GetTransform().GetPos();
	// move origin to center
	terrainOrigin -= glm::vec3(terrain->GetWidth(), 0, terrain->GetDepth()) / 2.f;

	aUB.SetUniform(0, terrainOrigin);
	aUB.SetUniform(1, GetTileSize(terrain)); // TODO: split into XY size instead of X size
	aUB.SetUniform(2, gridTiles.x);
	aUB.SetUniform(3, gridTiles.y);
	aUB.SetUniform(4, terrain->GetYScale());
}
