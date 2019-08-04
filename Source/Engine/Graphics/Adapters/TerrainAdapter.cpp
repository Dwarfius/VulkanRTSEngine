#include "Precomp.h"
#include "TerrainAdapter.h"

#include <Graphics/Camera.h>
#include <Graphics/UniformBlock.h>

#include "../../VisualObject.h"
#include "../../Terrain.h"
#include "../../Game.h"

TerrainAdapter::TerrainAdapter(const GameObject& aGO, const VisualObject& aVO)
	: UniformAdapter(aGO, aVO)
{
}

void TerrainAdapter::FillUniformBlock(const Camera& aCam, UniformBlock& aUB) const
{
	// Note: prefer to grab from VisualObject if possible, since the call will come from
	// VisualObject, thus memory will be in cache already

	const Terrain* terrain = Game::GetInstance()->GetTerrain(glm::vec3());
	const glm::vec3 scale = myVO.GetTransform().GetScale();
	const glm::vec3 size = scale * glm::vec3(terrain->GetWidth(), 0, terrain->GetDepth());
	const glm::vec3 tiles = glm::ceil(size / terrain->GetTileSize());
	const int gridWidth = static_cast<int>(glm::max(tiles.x, 1.f));
	const int gridDepth = static_cast<int>(glm::max(tiles.z, 1.f));
	const float tileSize = terrain->GetTileSize();
	const float halfTileSize = tileSize / 2.f;

	glm::vec3 terrainOrigin = myVO.GetTransform().GetPos();
	// move origin to center
	terrainOrigin -= glm::vec3(gridWidth * halfTileSize, 0, gridDepth * halfTileSize);

	aUB.SetUniform(0, terrainOrigin);
	aUB.SetUniform(1, tileSize); // TODO: split into XY size instead of X size
	aUB.SetUniform(2, gridWidth);
	aUB.SetUniform(3, gridDepth);
	aUB.SetUniform(4, terrain->GetYScale());
}
