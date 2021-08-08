#pragma once

#include <Graphics/UniformAdapter.h>

class GameObject;
class VisualObject;
class Terrain;

struct UniformAdapterSource : UniformAdapter::SourceData
{
	const GameObject* myGO; // should not be nullptr if Adapter relies on it
	const VisualObject& myVO;
};

struct TerrainUniformAdapterSource : UniformAdapterSource
{
	const Terrain& myTerrain;
};