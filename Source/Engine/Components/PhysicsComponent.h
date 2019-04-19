#pragma once

#include "ComponentBase.h"

class PhysicsEntity;
class PhysicsWorld;

// Component that handles the life-cycle of a physics entity
class PhysicsComponent : public ComponentBase
{
public:
	PhysicsComponent();
	virtual ~PhysicsComponent();

	bool IsInitialized() const { return myEntity != nullptr; }
	PhysicsEntity& GetPhysicsEntity() const { return *myEntity; }
	// PhysicsComponent claims ownership of the body and the shape
	void SetPhysicsEntity(PhysicsEntity* anEntity);
	void DeletePhysicsEntity();

	bool IsInWorld() const;
	void RequestAddToWorld(PhysicsWorld& aWorld) const;

private:
	// we own a phys entity, until we don't - see dtor
	PhysicsEntity* myEntity;
};