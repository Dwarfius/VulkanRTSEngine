#include "Common.h"
#include "PhysicsEntity.h"

#include "PhysicsWorld.h"

PhysicsEntity::PhysicsEntity(glm::uint64 anId, float aMass, btCollisionShape* aShape, btTransform aTransf)
	: myId(anId)
	, myShape(aShape)
	, myIsStatic(aMass == 0)
	, myIsSleeping(false)
	, myIsFrozen(false)
	, myWorld(nullptr)
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
	btDefaultMotionState* motionState = new btDefaultMotionState(aTransf);
	btVector3 localInertia(0, 0, 0);
	if (aMass > 0)
	{
		myShape->calculateLocalInertia(aMass, localInertia);
	}
	btRigidBody::btRigidBodyConstructionInfo constructInfo(aMass, motionState, myShape, localInertia);
	myBody = new btRigidBody(constructInfo);
}

PhysicsEntity::~PhysicsEntity()
{
	delete myBody->getMotionState();
	delete myBody;
	delete myShape;
}

glm::mat4 PhysicsEntity::GetTransform() const
{
	assert(myBody && myWorld);

	// convert to glm
	// Note: getWorldTransform() doesn't have CoM offset, will need to account for it if
	// we start providing it later
	glm::mat4 transf;
	myBody->getWorldTransform().getOpenGLMatrix(glm::value_ptr(transf));

	return transf;
}

glm::mat4 PhysicsEntity::GetTransformInterp() const
{
	assert(myBody && myWorld);

	// according to http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Stepping_The_World
	// Bullet interpolates stuff if world max step count is > 1, which we do have
	const btDefaultMotionState* motionState;
	motionState = static_cast<const btDefaultMotionState*>(myBody->getMotionState());

	// convert to glm
	// Note: m_graphicsWorldTrans might have CoM offset, but we don't provide one atm
	glm::mat4 transf;
	motionState->m_graphicsWorldTrans.getOpenGLMatrix(glm::value_ptr(transf));

	return transf;
}

void PhysicsEntity::SetTransform(glm::mat4 aTransf)
{
	assert(myBody && myWorld);

	btDefaultMotionState* motionState;
	motionState = static_cast<btDefaultMotionState*>(myBody->getMotionState());

	// convert to bullet and cache it
	motionState->m_graphicsWorldTrans.setFromOpenGLMatrix(glm::value_ptr(aTransf));
}