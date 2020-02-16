#pragma once

#include <Graphics/Resources/Model.h>

class PhysicsShapeHeightfield;

class Terrain
{
public:
	Terrain();

	void Load(AssetTracker& anAssetTracker, string aName, float aStep, float anYScale, float anUvScale);
	
	float GetHeight(glm::vec3 pos) const;
	glm::vec3 GetNormal(glm::vec3 pos) const;

	std::shared_ptr<PhysicsShapeHeightfield> CreatePhysicsShape();

	Handle<Model> GetModelHandle() const { return myModel; }

	float GetTileSize() const { return myStep * 64.f; /* 64 is the max tesselation level */ }
	float GetWidth() const { return myWidth * myStep; }
	float GetDepth() const { return myHeight * myStep; }
	float GetYScale() const { return myYScale; }

private:
	Handle<Model> myModel;

	// dimensions of the heightmap texture used
	int myWidth, myHeight;
	// distance between neighbouring terrain vertices
	float myStep;
	// controls how much texture should be scaled "vertically"
	float myYScale;

	void Normalize(Vertex* aVertices, size_t aVertCount, 
		Model::IndexType* aIndices, size_t aIndCount);

	// wraps the val value around [0;range] range
	float Wrap(float aVal, float aRange) const;

	// Preserve the heights so that physics can still reference to it
	vector<float> myHeightsCache;
};