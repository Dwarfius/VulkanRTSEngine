#pragma once

#ifdef ASSERT_MUTEX
#include <Core/Threading/AssertMutex.h>
#endif

// will need to extend this once I find out how bullet handles layers
enum CollisionLayers
{
	kLayerCount
};

class PhysicsWorld;
class PhysicsShapeBase;
class btRigidBody;
class btCollisionObject;

class IPhysControllable
{
private:
	friend struct EntityMotionState;
	friend class PhysicsEntity;
	virtual void SetPhysTransform(const glm::mat4& aTransf) = 0;
	virtual void GetPhysTransform(glm::mat4& aTranfs) const = 0;
};

class PhysicsEntity
{
public:
	enum State
	{
		NotInWorld,
		PendingAddition,
		InWorld,
		PendingRemoval,
	};
	// Creates an entity that is not linked to any game object, and has it's own transform
	PhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, const glm::mat4& aTransf);
	// Creates an entity that is linked to a game object, 
	// and synchronises the transform with it, with respect to the origin point
	PhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, IPhysControllable& anEntity, const glm::vec3& anOrigin);
	~PhysicsEntity();

	PhysicsEntity& operator=(const PhysicsEntity&) = delete;

	bool IsStatic() const { return myIsStatic; }
	bool IsDynamic() const { return !myIsStatic; } // declaring both for convenience
	// TODO: add kinemitic object support
	
	State GetState() const { return myState; }
	
	// is it currently ignoring the simulation
	bool IsFrozen() const { return myIsFrozen; }
	void Freeze() { myIsFrozen = true; }
	void UnFreeze() { myIsFrozen = false; }
	
	// is it currently simulating or not
	bool IsSleeping() const { return myIsSleeping; }
	void PutToSleep() { myIsSleeping = true; }
	void WakeUp() { myIsSleeping = false; }

	// Returns un-interpolated transform
	glm::mat4 GetTransform() const;
	// Returns interpolated transform
	glm::mat4 GetTransformInterp() const;
	// Updates the position of rigidbody. Does not take effect until stepped,
	// so if it needs to be fetched, use GetTransformInterp()
	void SetTransform(const glm::mat4& aTransf);

	float GetMass() const;

	// Thread-safe: add a force, will resolve on next phys step
	void AddForce(glm::vec3 aForce);

	// Not thread-safe: updates velocity immediatelly
	void SetVelocity(glm::vec3 aVelocity);

	// TODO: expose flags as an enum set
	void SetCollisionFlags(int aFlagSet);
	int GetCollisionFlags() const;

	const PhysicsShapeBase& GetShape() const { return *myShape; }

	// Method to schedule deletion of this entity
	// after the world has finished simulating it and removes
	// it from tracking
	void DeferDelete();

private:
	friend class PhysicsWorld;

	void ApplyForces();

	std::shared_ptr<PhysicsShapeBase> myShape;
	btCollisionObject* myBody;
	PhysicsWorld* myWorld;
	State myState;

#ifdef ASSERT_MUTEX
	AssertMutex myAccumForcesMutex;
#endif
	glm::vec3 myAccumForces;
	glm::vec3 myAccumTorque;

	bool myIsFrozen, myIsSleeping;
	bool myIsStatic;
};