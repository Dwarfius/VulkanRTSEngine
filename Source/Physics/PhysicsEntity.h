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
	PhysicsEntity(float aMass, const PhysicsShapeBase& aShape, const glm::mat4& aTransf);
	~PhysicsEntity();

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
	const glm::mat4& GetTransform() const;
	// Returns interpolated transform
	const glm::mat4& GetTransformInterp() const;
	// Updates the position of rigidbody. Does not take effect until stepped,
	// so if it needs to be fetched, use GetTransformInterp()
	void SetTransform(const glm::mat4& aTransf);

private:
	friend class PhysicsWorld;

	const PhysicsShapeBase& myShape;
	btRigidBody* myBody;
	PhysicsWorld* myWorld;

	bool myIsFrozen, myIsSleeping;
	bool myIsStatic;
};