#pragma once

class PhysicsEntity;
class btCollisionObject;

// TODO: investigate to template it on a lambda, to reduce the 
// amount of manual code writing
struct PhysicsCommand
{
	// make sure to add handlers of the types to the PhysicsWorld!
	enum Type
	{
		AddBody,
		RemoveBody,
		DeleteBody,
		ChangeBody,
		Count
	};

	explicit PhysicsCommand(Type aType)
		: myType(aType)
	{
	}

	virtual ~PhysicsCommand() = default;

	Type myType;
};

struct PhysicsCommandAddBody : PhysicsCommand
{
	explicit PhysicsCommandAddBody(PhysicsEntity* anEntity)
		: PhysicsCommand(Type::AddBody)
		, myEntity(anEntity)
	{
	}

	PhysicsEntity* myEntity;
};

struct PhysicsCommandRemoveBody : PhysicsCommand
{
	explicit PhysicsCommandRemoveBody(PhysicsEntity* anEntity)
		: PhysicsCommand(PhysicsCommand::RemoveBody)
		, myEntity(anEntity)
	{
	}

	PhysicsEntity* myEntity;
};

struct PhysicsCommandDeleteBody : PhysicsCommand
{
	explicit PhysicsCommandDeleteBody(PhysicsEntity* anEntity)
		: PhysicsCommand(PhysicsCommand::DeleteBody)
		, myEntity(anEntity)
	{
	}

	PhysicsEntity* myEntity;
};

struct PhysicsCommandChangeBody : PhysicsCommand
{
	explicit PhysicsCommandChangeBody(PhysicsEntity* anEntity, 
		btCollisionObject* anOldBody, bool anIsRigidbody)
		: PhysicsCommand(PhysicsCommand::ChangeBody)
		, myEntity(anEntity)
		, myOldBody(anOldBody)
		, myIsRigidbody(anIsRigidbody)
	{
	}

	PhysicsEntity* myEntity;
	btCollisionObject* myOldBody;
	bool myIsRigidbody;
};