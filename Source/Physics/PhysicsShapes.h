#pragma once

#include <Core/DataEnum.h>

class btCollisionShape;
class Serializer;

namespace Shapes
{
	struct AABB;
	struct Sphere;
}
// utility struct - ref-counting base of shapes
class PhysicsShapeBase
{
public:
	DATA_ENUM(Type, char,
		Invalid,
		Box,
		Sphere,
		Capsule,
		ConvexHull,
		Heightfield,
		Count
	);

public:
	PhysicsShapeBase();
	~PhysicsShapeBase();

	Shapes::AABB GetAABB(const glm::mat4& aTransform) const;
	Shapes::Sphere GetBoundingSphereLS() const;
	Type GetType() const { return myType; }

	void SetScale(glm::vec3 aScale);

protected:
	btCollisionShape* myShape;
	Type myType;

private:
	friend class PhysicsEntity;
	btCollisionShape* GetShape() const { return myShape; }
};

class PhysicsShapeBox : public PhysicsShapeBase
{
public:
	PhysicsShapeBox(glm::vec3 aHalfExtents);
	
	void SetHalfExtents(glm::vec3 aHalfExtents);
	glm::vec3 GetHalfExtents() const;
};

class PhysicsShapeSphere : public PhysicsShapeBase
{
public:
	PhysicsShapeSphere(float aRadius);

	void SetRadius(float aRadius);
	float GetRadius() const;
};

class PhysicsShapeCapsule : public PhysicsShapeBase
{
public:
	PhysicsShapeCapsule(float aRadius, float aHeight);
	
	void SetRadius(float aRadius);
	float GetRadius() const;

	void SetHeight(float aHeight);
	float GetHeight() const;
};

class PhysicsShapeConvexHull : public PhysicsShapeBase
{
public:
	// Takes in a vector of xyz points
	PhysicsShapeConvexHull(const std::vector<float>& aVertBuffer);
};

// simple heightfield shape which uses Y as up axis
class PhysicsShapeHeightfield : public PhysicsShapeBase
{
public:
	// Constructs a heightfield of aWidth x aLength of heights from aHeightBuffer
	// aMinHeight and aMaxHeight are used for fast calculating AABB
	PhysicsShapeHeightfield(int aWidth, int aDepth, std::vector<float>&& aHeigths, float aMinHeight, float aMaxHeight);

	// Moves aPos vertically to adjust for Bullet's
	// heightfield recentering of this terrain shape
	glm::vec3 AdjustPositionForRecenter(const glm::vec3& aPos) const;

	int GetWidth() const { return myWidth; }
	int GetDepth() const { return myDepth; }
	float GetMinHeight() const { return myMinHeight; }
	float GetMaxHeight() const { return myMaxHeight; }

	const std::vector<float>& GetHeights() const { return myHeights; }
	std::vector<float>& GetHeights() { return myHeights; }

private:
	std::vector<float> myHeights;
	// we store minHeight in order to be able to adjust the vertical
	// offset of the heightfield
	float myMinHeight;
	float myMaxHeight;
	int myWidth;
	int myDepth;
};