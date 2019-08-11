#pragma once

#include <Graphics/Model.h>

class PhysicsShapeHeightfield;

class Terrain
{
public:
	Terrain();

	void Load(AssetTracker& anAssetTracker, string aName, float aStep, float anYScale, float anUvScale);
	
	const Vertex* GetVertices() const { return myModel->GetVertices(); }
	size_t GetVertCount() const { return myModel->GetVertexCount(); }
	const Model::IndexType* GetIndices() const { return myModel->GetIndices(); }
	size_t GetIndCount() const { return myModel->GetIndexCount(); }

	float GetHeight(glm::vec3 pos) const;
	glm::vec3 GetNormal(glm::vec3 pos) const;

	std::shared_ptr<PhysicsShapeHeightfield> CreatePhysicsShape();

	Handle<Model> GetModelHandle() const { return myModel; }

	// TEMP - returning tile size which allows to have a pixel
	// per vertex mapping when using max (64) level of tesselation
	float GetTileSize() const { return 64.f; /*myStep * 64.f;*/ }
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

	void Normalize(vector<Vertex>& aVertices, const vector<Model::IndexType>& aIndices);

	// wraps the val value around [0;range] range
	float Wrap(float aVal, float aRange) const;

	// Preserve the heights so that physics can still reference to it
	vector<float> myHeightsCache;
};