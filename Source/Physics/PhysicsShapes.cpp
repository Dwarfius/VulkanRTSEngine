#include "Common.h"
#include "PhysicsShapes.h"

// ====================================================
PhysicsShapeBase::PhysicsShapeBase()
	: myShape(nullptr)
	, myType(Type::Invalid)
	, myRefCount(0) 
{
}

PhysicsShapeBase::~PhysicsShapeBase()
{ 
	assert(myRefCount == 0); 
}

AABB PhysicsShapeBase::GetAABB(const glm::mat4& aTransform) const
{
	assert(myShape);

	btTransform btTransf;
	btTransf.setFromOpenGLMatrix(glm::value_ptr(aTransform));

	btVector3 btMin, btMax;
	myShape->getAabb(btTransf, btMin, btMax);

	// copy them in one go
	AABB aabb;
	memcpy(glm::value_ptr(aabb.myMin), btMin.m_floats, 3 * sizeof(float));
	memcpy(glm::value_ptr(aabb.myMax), btMax.m_floats, 3 * sizeof(float));
	return aabb;
}

Sphere PhysicsShapeBase::GetBoundingSphereLS() const
{
	assert(myShape);

	Sphere sphere;
	btVector3 btCenter;
	myShape->getBoundingSphere(btCenter, sphere.myRadius);
	memcpy(glm::value_ptr(sphere.myCenter), btCenter.m_floats, 3 * sizeof(float));

	return sphere;
}

// ====================================================
PhysicsShapeBox::PhysicsShapeBox(glm::vec3 aHalfExtents)
	: PhysicsShapeBase()
{
	btVector3 halfExtents;
	memcpy(halfExtents.m_floats, glm::value_ptr(aHalfExtents), 3 * sizeof(float));
	myShape = new btBoxShape(halfExtents);
	myType = Type::Box;
}

// ====================================================
PhysicsShapeSphere::PhysicsShapeSphere(float aRadius)
	: PhysicsShapeBase()
{
	myShape = new btSphereShape(aRadius);
	myType = Type::Sphere;
}

// ====================================================
PhysicsShapeCapsule::PhysicsShapeCapsule(float aRadius, float aHeight)
	: PhysicsShapeBase()
{
	myShape = new btCapsuleShape(aRadius, aHeight);
	myType = Type::Capsule;
}

// ====================================================
PhysicsShapeConvexHull::PhysicsShapeConvexHull(const vector<float>& aVertBuffer)
	: PhysicsShapeBase()
{
	assert(aVertBuffer.size() % 3 == 0); // vertices of xyz configuration
	myShape = new btConvexHullShape(aVertBuffer.data(), aVertBuffer.size() / 3);
	myType = Type::ConvexHull;
}

// ====================================================
PhysicsShapeHeightfield::PhysicsShapeHeightfield(int aWidth, int aLength, const vector<float>& aHeightBuffer, float aMinHeight, float aMaxHeight)
	: PhysicsShapeBase()
{
	myShape = new btHeightfieldTerrainShape(aWidth, aLength, aHeightBuffer.data(), 1.f, aMinHeight, aMaxHeight, 1, PHY_FLOAT, false);
	myType = Type::Heightfield;
}