#include "Common.h"
#include "PhysicsEntity.h"

#include "PhysicsWorld.h"
#include "PhysicsShapes.h"
#include "PhysicsCommands.h"

PhysicsEntity::PhysicsEntity(float aMass, shared_ptr<PhysicsShapeBase> aShape, const glm::mat4& aTransf)
	: myShape(aShape)
	, myIsStatic(aMass == 0)
	, myIsSleeping(false)
	, myIsFrozen(false)
	, myWorld(nullptr)
	, myState(PhysicsEntity::NotInWorld)
{
	/* A Note about MotionState, from https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=12044
		btMotionState::getWorldTransform() is called for only two cases:
		(1) When you first add a body to the world, if it has a MotionState then 
		getWorldTransform() on that state is called to place the object within the world.
		(2) For each active kinematic object that has a MotionState: getWorldTransform() 
		is called on that state at the end of each simulaition substep. The idea here is 
		that the MotionState is supposed to fetch or compute the correct transform according 
		to whatever logic is moving it around within the physics engine.
		
		btMotionState::setWorldTransform() is called for each dynamic active object at the 
		end of each simulaition substep. You're supposed to implement that method such that 
		it copies the body's transform into your GameObject. Note: setWorldTransform() will 
		compute an interpolated transform when you're using fixed substeps but there is a 
		timestep remainder: this to reduce aliasing effects between the physics substep rate 
		and the render frame rate.

		btDefaultMotionState is a minimal implementation of the pure virtual btMotionState 
		interface.
	*/
	const btTransform transf = Utils::ConvertToBullet(aTransf);
	btCollisionShape* shape = myShape->GetShape();
	if (!myIsStatic)
	{
		btDefaultMotionState* motionState = new btDefaultMotionState();
		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(aMass, localInertia);
		btRigidBody::btRigidBodyConstructionInfo constructInfo(aMass, motionState, shape, localInertia);
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

PhysicsEntity::~PhysicsEntity()
{
	assert(myState == PhysicsEntity::NotInWorld);
	if (!myIsStatic)
	{
		delete static_cast<btRigidBody*>(myBody)->getMotionState();
	}
	delete myBody;
}

glm::mat4 PhysicsEntity::GetTransform() const
{
	assert(myBody);

	// Note: getWorldTransform() doesn't have CoM offset, will need to account for it if
	// we start providing it later
	return Utils::ConvertToGLM(myBody->getWorldTransform());
}

glm::mat4 PhysicsEntity::GetTransformInterp() const
{
	assert(myBody);
	assert(!myIsStatic);

	const btRigidBody* rigidBody = static_cast<const btRigidBody*>(myBody);

	assert(rigidBody->getMotionState());

	// according to http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Stepping_The_World
	// Bullet interpolates stuff if world max step count is > 1, which we do have
	const btDefaultMotionState* motionState;
	motionState = static_cast<const btDefaultMotionState*>(rigidBody->getMotionState());

	// Note: m_graphicsWorldTrans might have CoM offset, but we don't provide one atm
	return Utils::ConvertToGLM(motionState->m_graphicsWorldTrans);
}

// TODO: add a command version of this
void PhysicsEntity::SetTransform(const glm::mat4& aTransf)
{
	assert(myBody);

	// convert to bullet and cache it
	myBody->setWorldTransform(Utils::ConvertToBullet(aTransf));
}

// TODO: move this out of the PhysicsEntity
/*void PhysicsEntity::ScheduleAddForce(glm::vec3 aForce)
{
	assert(myWorld);
	assert(!Utils::IsNan(aForce));

	const PhysicsCommandAddForce* cmd = new PhysicsCommandAddForce(this, aForce);
	myWorld->EnqueueCommand(cmd);
}*/

void PhysicsEntity::AddForce(glm::vec3 aForce)
{
	assert(myBody);
	assert(!myIsStatic);
	assert(!Utils::IsNan(aForce));

	const btVector3 force = Utils::ConvertToBullet(aForce);
	btRigidBody* rigidBody = static_cast<btRigidBody*>(myBody);
	rigidBody->applyForce(force, btVector3(0.f, 0.f, 0.f));
}

// TODO: add a command version of this
void PhysicsEntity::SetVelocity(glm::vec3 aVelocity)
{
	assert(myBody);
	assert(!myIsStatic);
	assert(!Utils::IsNan(aVelocity));

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