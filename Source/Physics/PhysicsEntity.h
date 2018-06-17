#pragma once

// will need to extend this once I find out how bullet handles layers
enum CollisionLayers
{
	kLayerCount
};

class PhysicsWorld;

class PhysicsEntity
{
public:
	// PhysicsEntity handles lifetime of aShape
	PhysicsEntity(glm::uint64 anId, float aMass, btCollisionShape* aShape, btTransform aTransf);
	~PhysicsEntity();

	glm::uint64 GetId() const { return myId; }

	bool IsStatic() const { return myIsStatic; }
	bool IsDynamic() const { return !myIsStatic; } // declaring both for convenience
	
	bool IsInWorld() const { return myWorld != nullptr; }
	
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
	void SetTransform(glm::mat4 aTransf);

private:
	friend class PhysicsWorld;

	glm::uint64 myId;

	btCollisionShape* myShape;
	btRigidBody* myBody;
	PhysicsWorld* myWorld;

	bool myIsFrozen, myIsSleeping;
	bool myIsStatic;
};