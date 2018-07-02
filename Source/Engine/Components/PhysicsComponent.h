#pragma once

#include "ComponentBase.h"

class PhysicsEntity;
class PhysicsWorld;

class PhysicsComponent : public ComponentBase
{
public:
	PhysicsComponent();
	~PhysicsComponent();

	bool IsInitialized() const { return myEntity != nullptr; }
	PhysicsEntity& GetPhysicsEntity() const { return *myEntity; }
	// PhysicsComponent claims ownership of the body and the shape
	void SetPhysicsEntity(shared_ptr<PhysicsEntity> anEntity);

	bool IsInWorld() const;
	void RequestAddToWorld(PhysicsWorld& aWorld) const;

private:
	shared_ptr<PhysicsEntity> myEntity;
};