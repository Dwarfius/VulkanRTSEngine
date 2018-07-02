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
};

class PhysicsShapeSphere : public PhysicsShapeBase
{
public:
	PhysicsShapeSphere(float aRadius);
};

class PhysicsShapeCapsule : public PhysicsShapeBase
{
public:
	PhysicsShapeCapsule(float aRadius, float aHeight);
};

class PhysicsShapeConvexHull : public PhysicsShapeBase
{
public:
	// Takes in a vector of xyz points
	PhysicsShapeConvexHull(const vector<float>& aVertBuffer);
};

// simple heightfield shape which uses Y as up axis
class PhysicsShapeHeightfield : public PhysicsShapeBase
{
public:
	// Constructs a heightfield of aWidth x aLength of heights from aHeightBuffer
	// aMinHeight and aMaxHeight are used for fast calculating AABB
	PhysicsShapeHeightfield(int aWidth, int aLength, const vector<float>& aHeightBuffer, float aMinHeight, float aMaxHeight);
};