#include "Common.h"
#include "PhysicsWorld.h"
#include "PhysicsEntity.h"

PhysicsWorld::PhysicsWorld()
{
	myBroadphase = new btDbvtBroadphase();
	myConfiguration = new btDefaultCollisionConfiguration();
	myDispatcher = new btCollisionDispatcher(myConfiguration);
	mySolver = new btSequentialImpulseConstraintSolver();
	myWorld = new btDiscreteDynamicsWorld(myDispatcher, myBroadphase, mySolver, myConfiguration);
}

PhysicsWorld::~PhysicsWorld()
{
	delete myWorld;
	delete mySolver;
	delete myDispatcher;
	delete myConfiguration;
	delete myBroadphase;
}

void PhysicsWorld::AddEntity(PhysicsEntity* anEntity)
{
	// might need to synchronise this
	myWorld->addRigidBody(anEntity->myBody);
	anEntity->myWorld = this;
}

void PhysicsWorld::RemoveEntity(PhysicsEntity* anEntity)
{
	myWorld->removeRigidBody(anEntity->myBody);
	anEntity->myWorld = nullptr;
}

void PhysicsWorld::Simulate(float aDeltaTime)
{
	myWorld->stepSimulation(aDeltaTime, MaxSteps, FixedStepLength);
}