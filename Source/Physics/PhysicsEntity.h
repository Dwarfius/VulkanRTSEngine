#pragma once

#ifdef ASSERT_MUTEX
#include <Core/Threading/AssertMutex.h>
#endif

#include <Core/DataEnum.h>

// will need to extend this once I find out how bullet handles layers
enum CollisionLayers
{
	kLayerCount
};

class PhysicsWorld;
class PhysicsShapeBase;
class btRigidBody;
class btCollisionObject;
class btMotionState;
class Serializer;

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
	enum class State
	{
		NotInWorld,
		PendingAddition,
		InWorld,
		PendingRemoval,
	};

	DATA_ENUM(Type, char,
		Static,
		Dynamic,
		Trigger
	);

	// Creates an entity that is pending initialization. Requires setting up a shape
	// before adding to the physics world
	PhysicsEntity() = default;

	struct InitParams
	{
		Type myType = Type::Static;
		std::shared_ptr<PhysicsShapeBase> myShape;
		IPhysControllable* myEntity = nullptr;// optional
		glm::mat4 myTranfs{ 1 }; // expected if anEntity isn't set
		glm::vec3 myOffset{ 0 };
		float myMass{ 0 }; // only valid for Dynamic types
	};
	PhysicsEntity(const InitParams& aParams);
	~PhysicsEntity();

	PhysicsEntity& operator=(const PhysicsEntity&) = delete;

	Type GetType() const { return myType; }
	
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

	const PhysicsShapeBase* GetShape() const { return myShape.get(); }

	void SetOffset(glm::vec3 anOffset);
	glm::vec3 GetOffset() const { return myOffset; }

	// Method to schedule deletion of this entity
	// after the world has finished simulating it and removes
	// it from tracking
	void DeferDelete();

	void Serialize(Serializer& aSerializer, IPhysControllable* anEntity);

private:
	friend class PhysicsWorld;

	void ApplyForces();
	void SetMass(float aMass);
	void UpdateTransform();

	void CreateBody(const InitParams& aParams);
	void UpdateType(Type aType);
	void UpdateShape(const std::shared_ptr<PhysicsShapeBase>& aShape);
	void UpdateMass(float aMass);

	int GetOverlapCount() const;
	const PhysicsEntity* GetOverlapContact(int anIndex) const;

	std::shared_ptr<PhysicsShapeBase> myShape;
	btCollisionObject* myBody = nullptr;
	// Note: World will be set if we're in either of
	// PendingAddition, InWorld and PendingRemoval states
	PhysicsWorld* myWorld = nullptr;
	IPhysControllable* myPhysController = nullptr;
	State myState = State::NotInWorld;

#ifdef ASSERT_MUTEX
	AssertMutex myAccumForcesMutex;
#endif
	glm::vec3 myAccumForces{0};
	glm::vec3 myAccumTorque{0};
	glm::vec3 myOffset{0};

	Type myType = Type::Static;

	bool myIsFrozen = false;
	bool myIsSleeping = false;
};