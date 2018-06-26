#include "Common.h"
#include "PhysicsCommands.h"

#include "PhysicsEntity.h"

PhysicsCommand::PhysicsCommand(Type aType)
	: myType(aType)
{
}

PhysicsCommandAddBody::PhysicsCommandAddBody(PhysicsEntity* anEntity)
	: PhysicsCommand(PhysicsCommand::AddBody)
	, myEntity(anEntity)
{
}

PhysicsCommandRemoveBody::PhysicsCommandRemoveBody(PhysicsEntity* anEntity)
	: PhysicsCommand(PhysicsCommand::RemoveBody)
	, myEntity(anEntity)
{
}

PhysicsCommandAddForce::PhysicsCommandAddForce(PhysicsEntity* anEntity, glm::vec3 aForce)
	: PhysicsCommand(PhysicsCommand::AddForce)
	, myEntity(anEntity)
	, myForce(aForce)
{
}