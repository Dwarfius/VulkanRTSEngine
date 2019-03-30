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

}

void PhysicsComponent::SetPhysicsEntity(shared_ptr<PhysicsEntity> anEntity)
{
	myEntity = anEntity;
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