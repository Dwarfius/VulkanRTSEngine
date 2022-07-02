#include "Precomp.h"
#include "PhysicsEntity.h"

#include "PhysicsWorld.h"
#include "PhysicsShapes.h"
#include "PhysicsCommands.h"

#include <Core/Utils.h>
#include <Core/Resources/Serializer.h>

struct EntityMotionState : public btMotionState
{
private:
	// Non-owned
	IPhysControllable* myEntity;
	glm::vec3 myOrigin;

public:
	EntityMotionState(IPhysControllable* anEntity, const glm::vec3& anOrigin)
		: myEntity(anEntity)
		, myOrigin(anOrigin)
	{
	}

	void setWorldTransform(const btTransform& centerOfMassWorldTrans) override final
	{
		glm::mat4 transf = Utils::ConvertToGLM(centerOfMassWorldTrans);
		transf = glm::translate(transf, -myOrigin);
		myEntity->SetPhysTransform(transf);
	}

	void getWorldTransform(btTransform& centerOfMassWorldTrans) const override final
	{
		glm::mat4 transf;
		myEntity->GetPhysTransform(transf);
		transf = glm::translate(transf, myOrigin);
		centerOfMassWorldTrans = Utils::ConvertToBullet(transf);
	}

	void SetOrigin(const glm::vec3& anOrigin)
	{
		myOrigin = anOrigin;
	}
};

PhysicsEntity::PhysicsEntity(const InitParams& aParams)
{
	CreateBody(aParams);
}

PhysicsEntity::~PhysicsEntity()
{
	ASSERT(myState == State::NotInWorld);
	if (myType == Type::Dynamic)
	{
		delete static_cast<btRigidBody*>(myBody)->getMotionState();
	}
	delete myBody;
}

glm::mat4 PhysicsEntity::GetTransform() const
{
	ASSERT(myBody);

	// Note: getWorldTransform() doesn't have CoM offset, will need to account for it if
	// we start providing it later
	return Utils::ConvertToGLM(myBody->getWorldTransform());
}

glm::mat4 PhysicsEntity::GetTransformInterp() const
{
	ASSERT(myBody);
	ASSERT_STR(myType == Type::Dynamic, "Only Dynamic objects can move!");

	const btRigidBody* rigidBody = static_cast<const btRigidBody*>(myBody);

	ASSERT_STR(rigidBody->getMotionState(), "Motion state was not set-up");

	// according to http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Stepping_The_World
	// Bullet interpolates stuff if world max step count is > 1, which we do have
	const btDefaultMotionState* motionState;
	motionState = static_cast<const btDefaultMotionState*>(rigidBody->getMotionState());

	return Utils::ConvertToGLM(motionState->m_graphicsWorldTrans);
}

// TODO: add a command version of this
void PhysicsEntity::SetTransform(const glm::mat4& aTransf)
{
	ASSERT(myBody);

	// TODO: we have AssertRWMutex of myWorld available, except
	// that we can set these properties before the it was
	// added to world... need to check how it can be amended

	// convert to bullet and cache it
	myBody->setWorldTransform(Utils::ConvertToBullet(aTransf));
}

float PhysicsEntity::GetMass() const
{
	ASSERT(myBody);
	ASSERT_STR(myType == Type::Dynamic, "Only Dynamic objects have mass!");
	return static_cast<const btRigidBody*>(myBody)->getMass();
}

void PhysicsEntity::AddForce(glm::vec3 aForce)
{
	ASSERT(myBody);
	ASSERT_STR(myType == Type::Dynamic, "Only Dynamic objects can have force applied to!");
	ASSERT(!Utils::IsNan(aForce));

	{
		AssertLock lock(myAccumForcesMutex);
		myAccumForces += aForce;
	}
}

// TODO: add a command version of this
void PhysicsEntity::SetVelocity(glm::vec3 aVelocity)
{
	ASSERT(myBody);
	ASSERT_STR(myType == Type::Dynamic, "Only Dynamic objects can have velocity!");
	ASSERT(!Utils::IsNan(aVelocity));

	const btVector3 velocity = Utils::ConvertToBullet(aVelocity);
	btRigidBody* rigidBody = static_cast<btRigidBody*>(myBody);
	rigidBody->setLinearVelocity(velocity);
}

void PhysicsEntity::SetCollisionFlags(int aFlagSet)
{
	myBody->setCollisionFlags(aFlagSet); 
}

int PhysicsEntity::GetCollisionFlags() const 
{ 
	return myBody->getCollisionFlags();
}

void PhysicsEntity::SetOffset(glm::vec3 anOffset)
{
	if (myBody)
	{
		if (myType == Type::Dynamic)
		{
			btRigidBody* rigidBody = static_cast<btRigidBody*>(myBody);
			EntityMotionState* motionState = static_cast<EntityMotionState*>(rigidBody->getMotionState());
			motionState->SetOrigin(anOffset);
		}
		else
		{
			glm::vec3 offsetDelta = anOffset - myOffset;
			glm::mat4 transf = Utils::ConvertToGLM(myBody->getWorldTransform());
			transf = glm::translate(transf, offsetDelta);
			myBody->setWorldTransform(Utils::ConvertToBullet(transf));
		}
	}
	myOffset = anOffset;
}

void PhysicsEntity::DeferDelete()
{
	ASSERT(myWorld);
	myWorld->DeleteEntity(this);
}

void PhysicsEntity::Serialize(Serializer& aSerializer, IPhysControllable* anEntity)
{
	aSerializer.Serialize("myType", myType);

	PhysicsShapeBase::Type shapeType = myShape ?
		myShape->GetType() : PhysicsShapeBase::Type(PhysicsShapeBase::Type::Invalid);
	aSerializer.Serialize("myShape", shapeType);
	
	float mass = myBody && myType == Type::Dynamic 
		? static_cast<btRigidBody*>(myBody)->getMass() 
		: 0;
	float oldMass = mass;
	aSerializer.Serialize("myMass", mass);

	glm::vec3 offset = myOffset;
	glm::vec3 oldOffset = offset;
	aSerializer.Serialize("myOffset", offset);

	const bool createShape = aSerializer.IsReading()
		&& (!myShape || shapeType != myShape->GetType());

	if (shapeType != PhysicsShapeBase::Type::Invalid)
	{
		switch (shapeType)
		{
		case PhysicsShapeBase::Type::Box:
		{
			glm::vec3 halfExtents{ 0 };
			if (!createShape)
			{
				halfExtents = static_cast<PhysicsShapeBox&>(*myShape).GetHalfExtents();
			}
			aSerializer.Serialize("myHalfExtents", halfExtents);
			if (createShape)
			{
				myShape = std::make_shared<PhysicsShapeBox>(halfExtents);
			}
			else
			{
				static_cast<PhysicsShapeBox&>(*myShape).SetHalfExtents(halfExtents);
			}
			break;
		}
		case PhysicsShapeBase::Type::Sphere:
		{
			float radius = 0;
			if (!createShape)
			{
				radius = static_cast<PhysicsShapeSphere&>(*myShape).GetRadius();
			}
			aSerializer.Serialize("myRadius", radius);
			if (createShape)
			{
				myShape = std::make_shared<PhysicsShapeSphere>(radius);
			}
			else
			{
				static_cast<PhysicsShapeSphere&>(*myShape).SetRadius(radius);
			}
			break;
		}
		case PhysicsShapeBase::Type::Capsule:
		{
			float radius = 0;
			float height = 0;
			if (!createShape)
			{
				PhysicsShapeCapsule& capsule = static_cast<PhysicsShapeCapsule&>(*myShape);
				radius = capsule.GetRadius();
				height = capsule.GetHeight();
			}
			aSerializer.Serialize("myRadius", radius);
			aSerializer.Serialize("myHeight", height);
			if (createShape)
			{
				myShape = std::make_shared<PhysicsShapeCapsule>(radius, height);
			}
			else
			{
				PhysicsShapeCapsule& capsule = static_cast<PhysicsShapeCapsule&>(*myShape);
				capsule.SetRadius(radius);
				capsule.SetHeight(height);
			}
			break;
		}
		case PhysicsShapeBase::Type::Heightfield:
		{
			int width = 0;
			int depth = 0;
			float minHeight = 0;
			float maxHeight = 0;
			std::vector<float> empty;
			std::vector<float>& heights = !createShape ?
				static_cast<PhysicsShapeHeightfield&>(*myShape).GetHeights() : empty;
			if (!createShape)
			{
				PhysicsShapeHeightfield& heightfield = static_cast<PhysicsShapeHeightfield&>(*myShape);
				width = heightfield.GetWidth();
				depth = heightfield.GetDepth();
				minHeight = heightfield.GetMinHeight();
				maxHeight = heightfield.GetMaxHeight();
			}
			aSerializer.Serialize("myWidth", width);
			aSerializer.Serialize("myDepth", depth);
			aSerializer.Serialize("myMinHeight", minHeight);
			aSerializer.Serialize("myMaxHeight", maxHeight);
			aSerializer.Serialize("myHeights", heights);

			// Bullet doesn't expose heightfield data, so we can't modify existing shape
			// so we only support creating it once
			if (createShape)
			{
				myShape = std::make_shared<PhysicsShapeHeightfield>(width, depth, std::move(heights), minHeight, maxHeight);
			}
			break;
		}
		default:
			ASSERT(false);
		}
	}

	if (aSerializer.IsReading())
	{
		InitParams params;
		params.myType = myType;
		params.myShape = myShape;
		params.myEntity = anEntity;
		params.myTranfs = glm::mat4{ 1 };
		params.myOffset = offset;
		params.myMass = mass;
		CreateBody(params);
	}
}

void PhysicsEntity::ApplyForces()
{
	ASSERT_STR(myType == Type::Dynamic, "Only Dynamic objects can have foces applied to!");

	btVector3 force;
	btVector3 torque;

	{
		AssertLock lock(myAccumForcesMutex);

		force = Utils::ConvertToBullet(myAccumForces);
		torque = Utils::ConvertToBullet(myAccumTorque);
		myAccumForces = glm::vec3(0.f);
		myAccumTorque = glm::vec3(0.f);
	}

	// no forces - don't wake up
	if (force.isZero() && torque.isZero())
	{
		return;
	}

	btRigidBody* rigidBody = static_cast<btRigidBody*>(myBody);
	rigidBody->applyForce(force, btVector3(0.f, 0.f, 0.f));
	rigidBody->applyTorque(torque);

	if (!rigidBody->isActive())
	{
		// Bullet doesn't automatically wake up on force application
		// so have to do it manually
		rigidBody->activate();
	}
}

void PhysicsEntity::SetMass(float aMass)
{
	ASSERT_STR(myType == Type::Dynamic, "Only Dynamic objects have mass!");

	// recalculate local intertia
	btVector3 localInertia(0, 0, 0);
	myShape->GetShape()->calculateLocalInertia(aMass, localInertia);

	btRigidBody* rigidBody = static_cast<btRigidBody*>(myBody);
	rigidBody->setMassProps(aMass, localInertia);
}

void PhysicsEntity::UpdateTransform()
{
	if (myPhysController)
	{
		glm::mat4 transf;
		myPhysController->GetPhysTransform(transf);
		transf = glm::translate(transf, myOffset);
		SetTransform(transf);
	}
}

void PhysicsEntity::CreateBody(const InitParams& aParams)
{
	// input validation
	ASSERT_STR((aParams.myType == Type::Dynamic) == (aParams.myMass != 0.f),
		"Only Dynamic entities can have a mass!");

	// TODO: make this work with multiple calls
	ASSERT_STR(!myBody, "About to leak a body!");

	myType = aParams.myType;
	myOffset = aParams.myOffset;
	myPhysController = aParams.myEntity;
	btTransform startTransf;
	if (myPhysController)
	{
		glm::mat4 transf;
		myPhysController->GetPhysTransform(transf);
		transf = glm::translate(transf, myOffset);
		startTransf = Utils::ConvertToBullet(transf);
	}
	else
	{
		glm::mat4 transf = aParams.myTranfs;
		transf = glm::translate(transf, myOffset);
		startTransf = Utils::ConvertToBullet(transf);
	}

	myShape = aParams.myShape;
	btCollisionShape* shape = myShape->GetShape();

	switch (myType)
	{
	case Type::Static:
	{
		myBody = new btCollisionObject();
		myBody->setWorldTransform(startTransf);
		myBody->setCollisionShape(shape);
		break;
	}
	case Type::Dynamic:
	{
		EntityMotionState* motionState = nullptr;
		if (myPhysController)
		{
			motionState = new EntityMotionState(myPhysController, myOffset);
		}

		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(aParams.myMass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo constructInfo(aParams.myMass, motionState, shape, localInertia);
		constructInfo.m_startWorldTransform = startTransf;
		myBody = new btRigidBody(constructInfo);
		break;
	}
	case Type::Trigger:
	{
		myBody = new btGhostObject();
		myBody->setWorldTransform(startTransf);
		myBody->setCollisionShape(shape);
		break;
	}
	default:
		ASSERT(false);
	}
	myBody->setUserPointer(this);
}

int PhysicsEntity::GetOverlapCount() const
{
	ASSERT_STR(myType == Type::Trigger, "Unsupported call!");
	return static_cast<btGhostObject*>(myBody)->getNumOverlappingObjects();
}

const PhysicsEntity* PhysicsEntity::GetOverlapContact(int anIndex) const
{
	const btGhostObject* ghost = static_cast<btGhostObject*>(myBody);
	const btCollisionObject* other = ghost->getOverlappingObject(anIndex);
	return static_cast<const PhysicsEntity*>(other->getUserPointer());
}