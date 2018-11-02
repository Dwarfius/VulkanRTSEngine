#include "Precomp.h"
#include "PhysicsCommands.h"

#include "PhysicsEntity.h"

PhysicsCommand::PhysicsCommand(Type aType)
	: myType(aType)
{
}

PhysicsCommandAddBody::PhysicsCommandAddBody(weak_ptr<PhysicsEntity> anEntity)
	: PhysicsCommand(PhysicsCommand::AddBody)
	, myEntity(anEntity)
{
}

PhysicsCommandRemoveBody::PhysicsCommandRemoveBody(weak_ptr<PhysicsEntity> anEntity)
	: PhysicsCommand(PhysicsCommand::RemoveBody)
	, myEntity(anEntity)
{
}

PhysicsCommandAddForce::PhysicsCommandAddForce(weak_ptr<PhysicsEntity> anEntity, glm::vec3 aForce)
	: PhysicsCommand(PhysicsCommand::AddForce)
	, myEntity(anEntity)
	, myForce(aForce)
{
}