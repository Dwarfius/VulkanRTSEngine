#include "Precomp.h"
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

PhysicsCommandDeleteBody::PhysicsCommandDeleteBody(PhysicsEntity* anEntity)
	: PhysicsCommand(PhysicsCommand::DeleteBody)
	, myEntity(anEntity)
{
}