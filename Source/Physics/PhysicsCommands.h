#pragma once

class PhysicsEntity;

struct PhysicsCommand
{
	// make sure to add handlers of the types to the PhysicsWorld!
	enum Type
	{
		AddBody,
		RemoveBody,
		AddForce,
		SetTranform,
		Count
	};

	PhysicsCommand(Type aType);

	Type myType;
};

struct PhysicsCommandAddBody : PhysicsCommand
{
	PhysicsCommandAddBody(PhysicsEntity* anEntity);

	PhysicsEntity* myEntity;
};

struct PhysicsCommandRemoveBody : PhysicsCommand
{
	PhysicsCommandRemoveBody(PhysicsEntity* anEntity);

	PhysicsEntity* myEntity;
};

struct PhysicsCommandAddForce : PhysicsCommand
{
	PhysicsCommandAddForce(PhysicsEntity* anEntity, glm::vec3 aForce);

	PhysicsEntity* myEntity;
	glm::vec3 myForce;
};