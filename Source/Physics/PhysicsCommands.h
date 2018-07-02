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
	explicit PhysicsCommandAddBody(weak_ptr<PhysicsEntity> anEntity);

	weak_ptr<PhysicsEntity> myEntity;
};

struct PhysicsCommandRemoveBody : PhysicsCommand
{
	explicit PhysicsCommandRemoveBody(weak_ptr<PhysicsEntity> anEntity);

	weak_ptr<PhysicsEntity> myEntity;
};

struct PhysicsCommandAddForce : PhysicsCommand
{
	explicit PhysicsCommandAddForce(weak_ptr<PhysicsEntity> anEntity, glm::vec3 aForce);

	weak_ptr<PhysicsEntity> myEntity;
	glm::vec3 myForce;
};