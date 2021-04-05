#pragma once

class btCollisionShape;

// TODO: move them to common module
// ====================
// Utility structs
struct AABB
{
	glm::vec3 myMin, myMax;
};

struct Sphere
{
	glm::vec3 myCenter;
	float myRadius;
};
// ====================

// utility struct - ref-counting base of shapes
class PhysicsShapeBase
{
public:
	enum class Type
	{
		Invalid,
		Box,
		Sphere,
		Capsule,
		ConvexHull,
		Heightfield,
		Count
	};
public:
	PhysicsShapeBase();
	~PhysicsShapeBase();

	AABB GetAABB(const glm::mat4& aTransform) const;
	Sphere GetBoundingSphereLS() const;
	Type GetType() const { return myType; }

	void SetScale(glm::vec3 aScale);

protected:
	btCollisionShape* myShape;
	Type myType;

private:
	friend class PhysicsEntity;
	btCollisionShape* GetShape() const { return myShape; }

	friend class PhysicsShapeManager;
	int myRefCount;
};

class PhysicsShapeBox : public PhysicsShapeBase
{
public:
	PhysicsShapeBox(glm::vec3 aHalfExtents);
	glm::vec3 GetHalfExtents() const;
};

class PhysicsShapeSphere : public PhysicsShapeBase
{
public:
	PhysicsShapeSphere(float aRadius);
	float GetRadius() const;
};

class PhysicsShapeCapsule : public PhysicsShapeBase
{
public:
	PhysicsShapeCapsule(float aRadius, float aHeight);
	float GetRadius() const;
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
	PhysicsShapeHeightfield(int aWidth, int aLength, const float* aHeightBuffer, float aMinHeight, float aMaxHeight);

	// Moves aPos vertically to adjust for Bullet's
	// heightfield recentering of this terrain shape
	glm::vec3 AdjustPositionForRecenter(const glm::vec3& aPos) const;

private:
	// we store minHeight in order to be able to adjust the vertical
	// offset of the heightfield
	float myMinHeight;
};