#include "Common.h"
#include "PhysicsWorld.h"

#include "PhysicsEntity.h"
#include <BulletCollision\CollisionShapes\btTriangleShape.h>

PhysicsWorld::PhysicsWorld()
{
	myBroadphase = new btDbvtBroadphase();
	myConfiguration = new btDefaultCollisionConfiguration();
	myDispatcher = new btCollisionDispatcher(myConfiguration);
	mySolver = new btSequentialImpulseConstraintSolver();
	myWorld = new btDiscreteDynamicsWorld(myDispatcher, myBroadphase, mySolver, myConfiguration);

	myWorld->setDebugDrawer(new PhysicsDebugDrawer());
	myWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawContactPoints);

	myCommands.reserve(200);
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
	assert(anEntity->GetState() == PhysicsEntity::NotInWorld);
	assert(!anEntity->myWorld);
	anEntity->myState = PhysicsEntity::PendingAddition;
	// TODO: get rid of allocs/deallocs by using an internal recycler
	const PhysicsCommandAddBody* cmd = new PhysicsCommandAddBody(anEntity);
	EnqueueCommand(cmd);
}

void PhysicsWorld::RemoveEntity(PhysicsEntity* anEntity)
{
	assert(anEntity->GetState() == PhysicsEntity::InWorld);
	assert(anEntity->myWorld == this);
	anEntity->myState = PhysicsEntity::PendingRemoval;
	// TODO: get rid of allocs/deallocs by using an internal recycler
	const PhysicsCommandRemoveBody* cmd = new PhysicsCommandRemoveBody(anEntity);
	EnqueueCommand(cmd);
}

void PhysicsWorld::Simulate(float aDeltaTime)
{
	ResolveCommands();

	// TODO: instead of relying on the internal time resolver, I should manually step
	// simulation in order to accuratly call PrePhysicsFrame and PrePhysicsStep, otherwise they might
	// get called when simulation doesn't step
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

// little macro helper for switch cases to reduce the length
#define CALL_COMMAND_HANDLER(Type, Cmd) case PhysicsCommand::##Type: Type##Handler(static_cast<const PhysicsCommand##Type&>(Cmd)); break;
void PhysicsWorld::ResolveCommands()
{
	tbb::spin_mutex::scoped_lock lock(myCommandsLock);
	for (const PhysicsCommand* cmd : myCommands)
	{
		const PhysicsCommand& cmdRef = *cmd;
		switch (cmdRef.myType)
		{
			CALL_COMMAND_HANDLER(AddBody, cmdRef);
			CALL_COMMAND_HANDLER(RemoveBody, cmdRef);
			CALL_COMMAND_HANDLER(AddForce, cmdRef);
			case PhysicsCommand::Count: assert(false);
		}
		// TODO: get rid of allocs/deallocs by using an internal recycler
		delete cmd;
	}
	myCommands.clear();
}
#undef CALL_COMMAND_HANDLER

void PhysicsWorld::EnqueueCommand(const PhysicsCommand* aCmd)
{
	tbb::spin_mutex::scoped_lock lock(myCommandsLock);
	myCommands.push_back(aCmd);
}

void PhysicsWorld::AddBodyHandler(const PhysicsCommandAddBody& aCmd)
{
	assert(aCmd.myEntity);
	assert(!aCmd.myEntity->myWorld);
	assert(aCmd.myEntity->GetState() == PhysicsEntity::PendingAddition);

	// TODO: refactor this
	if (aCmd.myEntity->myIsStatic)
	{
		myWorld->addCollisionObject(aCmd.myEntity->myBody);
	}
	else
	{
		myWorld->addRigidBody(static_cast<btRigidBody*>(aCmd.myEntity->myBody));
	}
	aCmd.myEntity->myWorld = this;
	aCmd.myEntity->myState = PhysicsEntity::InWorld;
}

void PhysicsWorld::RemoveBodyHandler(const PhysicsCommandRemoveBody& aCmd)
{
	assert(aCmd.myEntity);
	assert(aCmd.myEntity->myWorld == this);
	assert(aCmd.myEntity->GetState() == PhysicsEntity::PendingRemoval);

	if (aCmd.myEntity->myIsStatic)
	{
		myWorld->removeCollisionObject(aCmd.myEntity->myBody);
	}
	else
	{
		myWorld->removeRigidBody(static_cast<btRigidBody*>(aCmd.myEntity->myBody));
	}
	aCmd.myEntity->myWorld = nullptr;
	aCmd.myEntity->myState = PhysicsEntity::NotInWorld;
}

void PhysicsWorld::AddForceHandler(const PhysicsCommandAddForce& aCmd)
{
	assert(aCmd.myEntity);
	assert(!aCmd.myEntity->myIsStatic);
	// even though we check during scheduling, it's nice to do it here to maybe catch a corruption
	assert(!Utils::IsNan(aCmd.myForce)); 

	const btVector3 force = Utils::ConvertToBullet(aCmd.myForce);
	static_cast<btRigidBody*>(aCmd.myEntity->myBody)->applyForce(force, btVector3(0.f, 0.f, 0.f));
}