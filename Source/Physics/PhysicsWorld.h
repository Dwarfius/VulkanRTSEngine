#pragma once

// TODO: Refactor away debug drawer include
#include "PhysicsDebugDrawer.h"
#include "PhysicsCommands.h"

class PhysicsEntity;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

// TODO: implement a command queue for phys world and entity commands
class PhysicsWorld
{
public:
	PhysicsWorld();
	~PhysicsWorld();

	// Adds entity to the world - not thread safe
	void AddEntity(PhysicsEntity* anEntity);
	// Removes entity from the world - not thread safe
	void RemoveEntity(PhysicsEntity* anEntity);

	void Simulate(float aDeltaTime);

	const vector<PhysicsDebugDrawer::LineDraw>& GetDebugLineCache() const;

private:
	const int MaxSteps = 4;
	const float FixedStepLength = 1.f / 30.f;

	void PrePhysicsFrame();
	void PostPhysicsFrame();

	// TODO: check out Bullet3
	btBroadphaseInterface* myBroadphase;
	btDefaultCollisionConfiguration* myConfiguration;
	btCollisionDispatcher* myDispatcher;
	btSequentialImpulseConstraintSolver* mySolver; // for now using default one, should try ODE quickstep solver later 
	btDiscreteDynamicsWorld* myWorld;
	
	tbb::spin_mutex myCommandsLock;
	// TODO: should probably have a reusable command cache, so that even it points to heap,
	// it'll be layed out continuously - should make more cache friendly
	vector<const PhysicsCommand*> myCommands;

	void ResolveCommands();

private:
	friend class PhysicsEntity;
	void EnqueueCommand(const PhysicsCommand* aCmd);

	// all command handlers
private:
	void AddBodyHandler(const PhysicsCommandAddBody& aCmd);
	void RemoveBodyHandler(const PhysicsCommandRemoveBody& aCmd);
	void AddForceHandler(const PhysicsCommandAddForce& aCmd);
};