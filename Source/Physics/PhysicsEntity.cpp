#include "Precomp.h"
#include "PhysicsEntity.h"

#include "PhysicsWorld.h"
#include "PhysicsShapes.h"
#include "PhysicsCommands.h"

#include <Core/Utils.h>

namespace
{
	struct EntityMotionState : public btMotionState
	{
	private:
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
			myEntity->SetTransform(transf);
		}

		void getWorldTransform(btTransform& centerOfMassWorldTrans) const override final
		{
			glm::mat4 transf;
			myEntity->GetTransform(transf);
			transf = glm::translate(transf, myOrigin);
			centerOfMassWorldTrans = Utils::ConvertToBullet(transf);
		}
	};
}

PhysicsEntity::PhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, const glm::mat4& aTransf)
	: myShape(aShape)
	, myIsStatic(aMass == 0)
	, myIsSleeping(false)
	, myIsFrozen(false)
	, myWorld(nullptr)
	, myState(PhysicsEntity::NotInWorld)
	, myAccumForces(0, 0, 0)
	, myAccumTorque(0, 0, 0)
{
	const btTransform transf = Utils::ConvertToBullet(aTransf);
	btCollisionShape* shape = myShape->GetShape();
	if (!myIsStatic)
	{
		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(aMass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo constructInfo(aMass, nullptr, shape, localInertia);
		constructInfo.m_startWorldTransform = transf;
		myBody = new btRigidBody(constructInfo);
	}
	else
	{
		myBody = new btCollisionObject();
		myBody->setWorldTransform(transf);
		myBody->setCollisionShape(shape);
	}

	myBody->setUserPointer(this);
}

PhysicsEntity::PhysicsEntity(float aMass, std::shared_ptr<PhysicsShapeBase> aShape, IPhysControllable& anEntity, const glm::vec3& anOrigin)
	: myShape(aShape)
	, myIsStatic(aMass == 0)
	, myIsSleeping(false)
	, myIsFrozen(false)
	, myWorld(nullptr)
	, myState(PhysicsEntity::NotInWorld)
	, myAccumForces(0, 0, 0)
	, myAccumTorque(0, 0, 0)
{
	btCollisionShape* shape = myShape->GetShape();
	if (!myIsStatic)
	{
		EntityMotionState* motionState = new EntityMotionState(&anEntity, anOrigin);
		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(aMass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo constructInfo(aMass, motionState, shape, localInertia);
		myBody = new btRigidBody(constructInfo);
	}
	else
	{
		glm::mat4 transf;
		anEntity.GetTransform(transf);
		transf = glm::translate(transf, anOrigin);
		myBody = new btCollisionObject();
		myBody->setWorldTransform(Utils::ConvertToBullet(transf));
		myBody->setCollisionShape(shape);
	}

	myBody->setUserPointer(this);
}

PhysicsEntity::~PhysicsEntity()
{
	ASSERT(myState == PhysicsEntity::NotInWorld);
	if (!myIsStatic)
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
	ASSERT_STR(!myIsStatic, "Static entities can't be interpolated");

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
	// added to world... need to check how it can be ammended

	// convert to bullet and cache it
	myBody->setWorldTransform(Utils::ConvertToBullet(aTransf));
}

void PhysicsEntity::AddForce(glm::vec3 aForce)
{
	ASSERT(myBody);
	ASSERT_STR(!myIsStatic, "Can't apply forces to a static object!");
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
	ASSERT_STR(!myIsStatic, "Static objects can't have velocity!");
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

void PhysicsEntity::DeferDelete()
{
	ASSERT(myWorld);
	myWorld->DeleteEntity(this);
}

void PhysicsEntity::ApplyForces()
{
	ASSERT_STR(!myIsStatic, "Can't apply forces to a static object!");

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