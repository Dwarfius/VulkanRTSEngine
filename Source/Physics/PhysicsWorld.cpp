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

	myWorld->setDebugDrawer(new PhysicsDebugDrawer());
	myWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
}

PhysicsWorld::~PhysicsWorld()
{
	delete myWorld->getDebugDrawer();
	delete myWorld;
	delete mySolver;
	delete myDispatcher;
	delete myConfiguration;
	delete myBroadphase;
}

void PhysicsWorld::AddEntity(PhysicsEntity* anEntity)
{
	assert(anEntity);
	assert(!anEntity->myWorld);

	// might need to synchronise this
	myWorld->addRigidBody(anEntity->myBody);
	anEntity->myWorld = this;
}

void PhysicsWorld::RemoveEntity(PhysicsEntity* anEntity)
{
	assert(anEntity);
	assert(anEntity->myWorld == this);

	myWorld->removeRigidBody(anEntity->myBody);
	anEntity->myWorld = nullptr;
}

void PhysicsWorld::Simulate(float aDeltaTime)
{
	PrePhysicsFrame();
	// even if we don't have enough deltaTime this frame, Bullet will avoid stepping
	// the simulation, but it will update the motion states, thus achieving interpolation
	myWorld->stepSimulation(aDeltaTime, MaxSteps, FixedStepLength);
	myWorld->debugDrawWorld();
	PostPhysicsFrame();
}

const vector<PhysicsDebugDrawer::LineDraw>& PhysicsWorld::GetDebugLineCache() const
{
	return static_cast<PhysicsDebugDrawer*>(myWorld->getDebugDrawer())->GetLineCache();
}
 
void PhysicsWorld::PrePhysicsFrame()
{

}

void PhysicsWorld::PostPhysicsFrame()
{

}