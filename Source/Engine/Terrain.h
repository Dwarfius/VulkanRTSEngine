#pragma once

#include <Graphics/Resources/Model.h>

class PhysicsShapeHeightfield;
class Texture;

class Terrain
{
public:
	Terrain();

	void Load(Handle<Texture> aTexture, float aStep, float anYScale);
	void Generate(glm::uvec2 aSize, float aStep, float anYScale);
	
	float GetHeight(glm::vec3 pos) const;
	glm::vec3 GetNormal(glm::vec3 pos) const;

	std::shared_ptr<PhysicsShapeHeightfield> CreatePhysicsShape();

	Handle<Texture> GetTextureHandle() const { return myTexture; }

	float GetWidth() const { return (myWidth - 1) * myStep; }
	float GetDepth() const { return (myHeight - 1) * myStep; }
	float GetYScale() const { return myYScale; }

private:
	static Vertex* GenerateVerticesFromData(const glm::uvec2& aSize, float aStep, const std::function<float(size_t)>& aReadCB,
										float& aMinHeight, float& aMaxHeight);
	static Model::IndexType* GenerateIndices(const glm::uvec2& aSize);
	static Handle<Model> CreateModel(const glm::vec2& aFullSize, float aMinHeight, float aMaxHeight,
		const Vertex* aVertices, size_t aVertCount,
		const Model::IndexType* aIndices, size_t aIndCount);

	void Normalize(Vertex* aVertices, size_t aVertCount,
		Model::IndexType* aIndices, size_t aIndCount);

	// TODO: get rid of model!
	Handle<Model> myModel;
	Handle<Texture> myTexture;

	// dimensions of the heightmap texture used
	uint32_t myWidth, myHeight;
	// distance between neighbouring terrain vertices
	float myStep;
	// controls how much texture should be scaled "vertically"
	float myYScale;

	// Preserve the heights so that physics can still reference to it
	std::vector<float> myHeightsCache;
};