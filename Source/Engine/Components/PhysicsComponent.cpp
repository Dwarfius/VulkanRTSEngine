#include "Precomp.h"
#include "PhysicsComponent.h"

#include "Game.h"
#include "GameObject.h"

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>
#include <Core/Resources/Serializer.h>

PhysicsComponent::~PhysicsComponent()
{
	if (myEntity)
	{
		DeletePhysicsEntity();
	}
}

void PhysicsComponent::SetOrigin(glm::vec3 anOrigin)
{
	ASSERT_STR(myEntity, "Missing entity!");
	myEntity->SetOffset(anOrigin);
}

glm::vec3 PhysicsComponent::GetOrigin() const
{
	ASSERT_STR(myEntity, "Missing entity!");
	return myEntity->GetOffset();
}

void PhysicsComponent::CreatePhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, glm::vec3 anOrigin)
{
	ASSERT_STR(myOwner, "There's no owner - did you forget to add the component to the game object?");
	ASSERT_STR(!myEntity, "About to leak entity!");

	PhysicsEntity::InitParams params;
	params.myType = aMass == 0.f ? PhysicsEntity::Type::Static : PhysicsEntity::Type::Dynamic;
	params.myShape = aShape;
	params.myEntity = myOwner;
	params.myOffset = anOrigin;
	params.myMass = aMass;
	myEntity = new PhysicsEntity(params);
}

void PhysicsComponent::CreateOwnerlessPhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, const glm::mat4& aTransf)
{
	ASSERT_STR(!myEntity, "About to leak entity!");
	PhysicsEntity::InitParams params;
	params.myType = aMass == 0.f ? PhysicsEntity::Type::Static : PhysicsEntity::Type::Dynamic;
	params.myShape = aShape;
	params.myEntity = nullptr;
	params.myTranfs = aTransf;
	params.myMass = aMass;
	myEntity = new PhysicsEntity(params);
}

void PhysicsComponent::CreateTriggerEntity(std::shared_ptr<PhysicsShapeBase> aShape, glm::vec3 anOrigin)
{
	ASSERT_STR(myOwner, "There's no owner - did you forget to add the component to the game object?");
	ASSERT_STR(!myEntity, "About to leak entity!");

	PhysicsEntity::InitParams params;
	params.myType = PhysicsEntity::Type::Trigger;
	params.myShape = aShape;
	params.myEntity = myOwner;
	params.myOffset = anOrigin;
	myEntity = new PhysicsEntity(params);
}

void PhysicsComponent::CreateOwnerlessTriggerEntity(std::shared_ptr<PhysicsShapeBase> aShape, const glm::mat4& aTransf)
{
	ASSERT_STR(!myEntity, "About to leak entity!");

	PhysicsEntity::InitParams params;
	params.myType = PhysicsEntity::Type::Trigger;
	params.myShape = aShape;
	params.myEntity = nullptr;
	params.myTranfs = aTransf;
	myEntity = new PhysicsEntity(params);
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
		isInWorld = myEntity->GetState() == PhysicsEntity::State::InWorld;
	}

	return isInWorld;
}

void PhysicsComponent::RequestAddToWorld(PhysicsWorld& aWorld) const
{
	ASSERT(myEntity);
	aWorld.AddEntity(myEntity);
}

void PhysicsComponent::Serialize(Serializer& aSerializer)
{
	if (Serializer::ObjectScope entityScope{ aSerializer, "myPhysEntity" })
	{
		if (!myEntity)
		{
			myEntity = new PhysicsEntity();
		}
		myEntity->Serialize(aSerializer, myOwner);
	}
}