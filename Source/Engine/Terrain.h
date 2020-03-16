#pragma once

#include <Graphics/Resources/Model.h>

class PhysicsShapeHeightfield;
class Texture;

class Terrain
{
public:
	Terrain();

	void Load(Handle<Texture> aTexture, float aStep, float anYScale);
	void Generate(glm::ivec2 aSize, float aStep, float anYScale);
	
	float GetHeight(glm::vec3 pos) const;
	glm::vec3 GetNormal(glm::vec3 pos) const;

	std::shared_ptr<PhysicsShapeHeightfield> CreatePhysicsShape();

	Handle<Model> GetModelHandle() const { return myModel; }
	Handle<Texture> GetTextureHandle() const { return myTexture; }

	float GetTileSize() const { return myStep * 64.f; /* 64 is the max tesselation level */ }
	float GetWidth() const { return myWidth * myStep; }
	float GetDepth() const { return myHeight * myStep; }
	float GetYScale() const { return myYScale; }

private:
	static Vertex* GenerateVerticesFromData(const glm::ivec2& aSize, float aStep, const std::function<float(size_t)>& aReadCB,
										float& aMinHeight, float& aMaxHeight);
	static Model::IndexType* GenerateIndices(const glm::ivec2& aSize);
	static Handle<Model> CreateModel(const glm::vec2& aFullSize, float aMinHeight, float aMaxHeight,
		const Vertex* aVertices, size_t aVertCount,
		const Model::IndexType* aIndices, size_t aIndCount);

	void Normalize(Vertex* aVertices, size_t aVertCount,
		Model::IndexType* aIndices, size_t aIndCount);

	Handle<Model> myModel;
	Handle<Texture> myTexture;

	// dimensions of the heightmap texture used
	int myWidth, myHeight;
	// distance between neighbouring terrain vertices
	float myStep;
	// controls how much texture should be scaled "vertically"
	float myYScale;

	

	// Preserve the heights so that physics can still reference to it
	std::vector<float> myHeightsCache;
};