#include "Precomp.h"
#include "PhysicsComponent.h"

#include "Game.h"
#include "GameObject.h"

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsShapes.h>
#include <Core/Resources/Serializer.h>

PhysicsComponent::PhysicsComponent()
	: myEntity(nullptr)
	, myOrigin(0, 0, 0)
{
}

PhysicsComponent::~PhysicsComponent()
{
	if (myEntity)
	{
		DeletePhysicsEntity();
	}
}

void PhysicsComponent::CreatePhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape)
{
	ASSERT_STR(myOwner, "There's no owner - did you forget to add the component to the game object?");
	ASSERT_STR(!myEntity, "About to leak entity!");
	myEntity = new PhysicsEntity(aMass, aShape, *myOwner, myOrigin);
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

void PhysicsComponent::Serialize(Serializer& aSerializer)
{
	PhysicsShapeBase::Type shapeType = myEntity ? 
		myEntity->GetShape().GetType() : PhysicsShapeBase::Type::Invalid;
	aSerializer.Serialize("myShape", shapeType);

	float mass = myEntity && myEntity->IsDynamic() ? myEntity->GetMass() : 0;
	aSerializer.Serialize("myMass", mass);
	if (aSerializer.IsReading())
	{
		if (myEntity)
		{
			DeletePhysicsEntity();
		}
		std::shared_ptr<PhysicsShapeBase> shape;
		switch (shapeType)
		{
		case PhysicsShapeBase::Type::Box:
		{
			glm::vec3 halfExtents;
			aSerializer.Serialize("myHalfExtents", halfExtents);
			shape = std::make_shared<PhysicsShapeBox>(halfExtents);
			break;
		}
		case PhysicsShapeBase::Type::Sphere:
		{
			float radius = 0;
			aSerializer.Serialize("myRadius", radius);
			shape = std::make_shared<PhysicsShapeSphere>(radius);
			break;
		}
		case PhysicsShapeBase::Type::Capsule:
		{
			float radius = 0;
			aSerializer.Serialize("myRadius", radius);
			float height = 0;
			aSerializer.Serialize("myHeight", height);
			shape = std::make_shared<PhysicsShapeCapsule>(radius, height);
			break;
		}
		default:
			ASSERT(false);
		}
		CreatePhysicsEntity(mass, shape);
	}
	else
	{
		if (myEntity)
		{
			const PhysicsShapeBase& baseShape = myEntity->GetShape();
			switch (shapeType)
			{
			case PhysicsShapeBase::Type::Box:
			{
				const PhysicsShapeBox& shape = static_cast<const PhysicsShapeBox&>(baseShape);
				aSerializer.Serialize("myHalfExtents", shape.GetHalfExtents());
				break;
			}
			case PhysicsShapeBase::Type::Sphere:
			{
				const PhysicsShapeSphere& shape = static_cast<const PhysicsShapeSphere&>(baseShape);
				float radius = shape.GetRadius();
				aSerializer.Serialize("myRadius", radius);
				break;
			}
			case PhysicsShapeBase::Type::Capsule:
			{
				const PhysicsShapeCapsule& shape = static_cast<const PhysicsShapeCapsule&>(baseShape);
				float radius = shape.GetRadius();
				aSerializer.Serialize("myRadius", radius);
				float height = shape.GetHeight();
				aSerializer.Serialize("myHeight", height);
				break;
			}
			default:
				ASSERT(false);
			}
		}
	}

	aSerializer.Serialize("myOrigin", myOrigin);
}