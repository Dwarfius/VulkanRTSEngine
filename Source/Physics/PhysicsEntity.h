#pragma once

// will need to extend this once I find out how bullet handles layers
enum CollisionLayers
{
	kLayerCount
};

class PhysicsWorld;
class PhysicsShapeBase;
class btRigidBody;

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
	PhysicsEntity(float aMass, const PhysicsShapeBase& aShape, const glm::mat4& aTransf);
	~PhysicsEntity();

	bool IsStatic() const { return myIsStatic; }
	bool IsDynamic() const { return !myIsStatic; } // declaring both for convenience
	
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

	// Thread-safe: adds a force to the entity before the next simulation step
	void ScheduleAddForce(glm::vec3 aForce);
	// Not thread-safe: add a force immediatelly
	void AddForce(glm::vec3 aForce);

	// Not thread-safe: updates velocity immediatelly
	void SetVelocity(glm::vec3 aVelocity);

private:
	friend class PhysicsWorld;

	const PhysicsShapeBase& myShape;
	btRigidBody* myBody;
	PhysicsWorld* myWorld;
	State myState;

	bool myIsFrozen, myIsSleeping;
	bool myIsStatic;
};