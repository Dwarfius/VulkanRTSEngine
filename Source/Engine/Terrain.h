#pragma once

#include <Core/RefCounted.h>

class PhysicsShapeHeightfield;
class Texture;

class Terrain
{
public:
	Terrain();

	void Load(Handle<Texture> aTexture, float aStep, float anYScale);
	void Generate(glm::uvec2 aSize, float aStep, float anYScale);
	
	// pos is in local space
	float GetHeight(glm::vec3 aLocalPos) const;

	Handle<Texture> GetTextureHandle() const { return myTexture; }

	float GetWidth() const { return (myWidth - 1) * myStep; }
	float GetDepth() const { return (myHeight - 1) * myStep; }
	float GetYScale() const { return myYScale; }

	std::shared_ptr<PhysicsShapeHeightfield> GetPhysShape() const { return myHeightfield; }

private:
	float GetHeightAtVert(uint32_t aX, uint32_t aY) const;

	Handle<Texture> myTexture;

	// dimensions of the heightmap texture used
	uint32_t myWidth, myHeight;
	// vertical extent of the terrain
	float myMinHeight, myMaxHeight;
	// distance between neighbouring terrain vertices
	float myStep;
	// controls how much texture should be scaled "vertically"
	float myYScale;

	// TODO: remove this from the terrain
	std::shared_ptr<PhysicsShapeHeightfield> myHeightfield;
};