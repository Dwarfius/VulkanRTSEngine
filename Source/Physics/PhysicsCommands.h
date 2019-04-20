#pragma once

class PhysicsEntity;

// TODO: investigate to template it on a lambda, to reduce the 
// amount of manual code writing
struct PhysicsCommand
{
	// make sure to add handlers of the types to the PhysicsWorld!
	enum Type
	{
		AddBody,
		RemoveBody,
		SetTranform,
		DeleteBody,
		Count
	};

	PhysicsCommand(Type aType);

	Type myType;
};

struct PhysicsCommandAddBody : PhysicsCommand
{
	explicit PhysicsCommandAddBody(PhysicsEntity* anEntity);

	PhysicsEntity* myEntity;
};

struct PhysicsCommandRemoveBody : PhysicsCommand
{
	explicit PhysicsCommandRemoveBody(PhysicsEntity* anEntity);

	PhysicsEntity* myEntity;
};

struct PhysicsCommandDeleteBody : PhysicsCommand
{
	explicit PhysicsCommandDeleteBody(PhysicsEntity* anEntity);

	PhysicsEntity* myEntity;
};