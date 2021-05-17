#pragma once

#include <Core/RefCounted.h>

class PhysicsShapeHeightfield;
class Texture;
class GameObject;
class PhysicsWorld;

class Terrain
{
public:
	Terrain();

	void Load(Handle<Texture> aTexture, float aStep, float anYScale);
	void Generate(glm::uvec2 aSize, float aStep, float anYScale);
	
	// pos is in local space
	float GetHeight(glm::vec3 pos) const;
	// pos is in local space
	glm::vec3 GetNormal(glm::vec3 pos) const;

	void AddPhysicsEntity(GameObject& aGO, PhysicsWorld& aPhysWorld);

	Handle<Texture> GetTextureHandle() const { return myTexture; }

	float GetWidth() const { return (myWidth - 1) * myStep; }
	float GetDepth() const { return (myHeight - 1) * myStep; }
	float GetYScale() const { return myYScale; }

private:
	float GetHeightAtVert(uint32_t x, uint32_t y) const;

	Handle<Texture> myTexture;

	// dimensions of the heightmap texture used
	uint32_t myWidth, myHeight;
	// vertical extent of the terrain
	float myMinHeight, myMaxHeight;
	// distance between neighbouring terrain vertices
	float myStep;
	// controls how much texture should be scaled "vertically"
	float myYScale;

	std::shared_ptr<PhysicsShapeHeightfield> myHeightfield;
};