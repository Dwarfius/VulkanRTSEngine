#pragma once

#include "ComponentBase.h"

class PhysicsEntity;
class PhysicsWorld;
class PhysicsShapeBase;

// Component that handles the life-cycle of a physics entity
class PhysicsComponent : public ComponentBase
{
public:
	PhysicsComponent();
	virtual ~PhysicsComponent();

	bool IsInitialized() const { return myEntity != nullptr; }
	PhysicsEntity& GetPhysicsEntity() const { return *myEntity; }

	// A mass of 0 denotes a static phys entity!
	void CreatePhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape);
	void DeletePhysicsEntity();

	bool IsInWorld() const;
	void RequestAddToWorld(PhysicsWorld& aWorld) const;

private:
	// we own a phys entity, until we don't - see dtor
	PhysicsEntity* myEntity;
};