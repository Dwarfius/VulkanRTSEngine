#include "Precomp.h"
#include "PhysicsComponent.h"

#include "../Game.h"

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>

PhysicsComponent::PhysicsComponent()
	: myEntity(nullptr)
{
}

PhysicsComponent::~PhysicsComponent()
{
	if (myEntity)
	{
		DeletePhysicsEntity();
	}
}

void PhysicsComponent::SetPhysicsEntity(PhysicsEntity* anEntity)
{
	myEntity = anEntity;
}

void PhysicsComponent::DeletePhysicsEntity()
{
	ASSERT(myEntity);

	if (myEntity->GetState() == PhysicsEntity::State::NotInWorld)
	{
		// if it's not in the world it's simple - just delete the entity
		delete myEntity;
	}
	else
	{
		// but, if it's not, then it has to be on the way to the world
		// or removed from the world. We'll transfer ownership to the
		// physics world until it gets stepped
		// until that happens, everyone else will only be able to see
		// that it doesn't exist anymore
		myEntity->DeferDelete();
	}
	myEntity = nullptr;
}

bool PhysicsComponent::IsInWorld() const
{
	bool isInWorld = false;

	if (IsInitialized())
	{
		isInWorld = myEntity->GetState() == PhysicsEntity::InWorld;
	}

	return isInWorld;
}

void PhysicsComponent::RequestAddToWorld(PhysicsWorld& aWorld) const
{
	ASSERT(myEntity);
	aWorld.AddEntity(myEntity);
}