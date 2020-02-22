#include "Precomp.h"
#include "PhysicsShapes.h"

// ====================================================
PhysicsShapeBase::PhysicsShapeBase()
	: myShape(nullptr)
	, myType(Type::Invalid)
	, myRefCount(0) // TODO: currently does nothing, need to implement or remove
{
}

PhysicsShapeBase::~PhysicsShapeBase()
{ 
	ASSERT(myRefCount == 0); 
	ASSERT_STR(myShape, "Don't create blank shapes if you don't need them"); 
	delete myShape;
}

AABB PhysicsShapeBase::GetAABB(const glm::mat4& aTransform) const
{
	ASSERT(myShape);

	btTransform btTransf;
	btTransf.setFromOpenGLMatrix(glm::value_ptr(aTransform));

	btVector3 btMin, btMax;
	myShape->getAabb(btTransf, btMin, btMax);

	// copy them in one go
	AABB aabb;
	aabb.myMin = Utils::ConvertToGLM(btMin);
	aabb.myMax = Utils::ConvertToGLM(btMax);
	return aabb;
}

Sphere PhysicsShapeBase::GetBoundingSphereLS() const
{
	ASSERT(myShape);

	Sphere sphere;
	btVector3 btCenter;
	myShape->getBoundingSphere(btCenter, sphere.myRadius);
	sphere.myCenter = Utils::ConvertToGLM(btCenter);

	return sphere;
}

void PhysicsShapeBase::SetScale(glm::vec3 aScale)
{
	myShape->setLocalScaling(Utils::ConvertToBullet(aScale));
}

// ====================================================
PhysicsShapeBox::PhysicsShapeBox(glm::vec3 aHalfExtents)
	: PhysicsShapeBase()
{
	btVector3 halfExtents = Utils::ConvertToBullet(aHalfExtents);
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
PhysicsShapeConvexHull::PhysicsShapeConvexHull(const std::vector<float>& aVertBuffer)
	: PhysicsShapeBase()
{
	ASSERT_STR(aVertBuffer.size() % 3 == 0, "Convex hull shape requires XYZ-form vertices");
	int pointCount = static_cast<int>(aVertBuffer.size() / 3);
	myShape = new btConvexHullShape(aVertBuffer.data(), pointCount);
	myType = Type::ConvexHull;
}

// ====================================================
PhysicsShapeHeightfield::PhysicsShapeHeightfield(int aWidth, int aLength, const std::vector<float>& aHeightBuffer, float aMinHeight, float aMaxHeight)
	: PhysicsShapeBase()
{
	myShape = new btHeightfieldTerrainShape(aWidth, aLength, aHeightBuffer.data(), 1.f, aMinHeight, aMaxHeight, 1, PHY_FLOAT, false);
	myType = Type::Heightfield;
}