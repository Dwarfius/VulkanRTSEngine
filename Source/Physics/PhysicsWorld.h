#pragma once

#include "PhysicsCommands.h"

#include <Core/Threading/AssertRWMutex.h>

class PhysicsEntity;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btGhostPairCallback;
class DebugDrawer;

class PhysicsWorld
{
public:
	// A callback to support implementing per-world systems
	// that want to provide physics simulation
	class ISymCallbackListener
	{
	public:
		virtual void OnPrePhysicsStep(float aDeltaTime) = 0;
		virtual void OnPostPhysicsStep(float aDeltaTime) = 0;
		// Called once per entity pair, before OnPostPhysicsStep
		virtual void OnTriggerCallback(const PhysicsEntity& aLeft, const PhysicsEntity& aRight) = 0;
	};

public:
	PhysicsWorld();
	~PhysicsWorld();

	// Adds entity to the world - threadsafe
	void AddEntity(PhysicsEntity* anEntity);
	// Removes entity from the world - threadsafe
	void RemoveEntity(PhysicsEntity* anEntity);
	// Deletes entity and cleans up references - threadsafe
	void DeleteEntity(PhysicsEntity* anEntity);

	void Simulate(float aDeltaTime);

	// Makes a raycast and returns the closest hit, if there is one
	bool RaycastClosest(glm::vec3 aFrom, glm::vec3 aDir, float aDist, PhysicsEntity*& aHitEntity) const;
	// Makes a raycast and returns the closest hit, if there is one
	bool RaycastClosest(glm::vec3 aFrom, glm::vec3 aTo, PhysicsEntity*& aHitEntity) const;

	// TODO: use stack-allocated aligned vector to match up with btAlignedArray
	// Makes a raycast and returns all the hits
	bool Raycast(glm::vec3 aFrom, glm::vec3 aDir, float aDist, std::vector<PhysicsEntity*>& anAllHits) const;
	// Makes a raycast and returns all the hits
	bool Raycast(glm::vec3 aFrom, glm::vec3 aTo, std::vector<PhysicsEntity*>& anAllHits) const;

	// TODO: refactor this away
	const DebugDrawer& GetDebugDrawer() const;

	bool IsDebugDrawingEnabled() const;
	void SetDebugDrawing(bool aIsEnabled);

	// Subscribes a system to Pre/PostPhys callbacks
	// Not thread safe
	void AddPhysSystem(ISymCallbackListener* aSystem);

private:
	constexpr static int kMaxSteps = 4;
	constexpr static float kFixedStepLength = 1.f / 30.f;

	std::vector<PhysicsEntity*> myStaticEntities;
	std::vector<PhysicsEntity*> myDynamicEntities;
	std::vector<PhysicsEntity*> myTriggers;
	std::vector<ISymCallbackListener*> myPhysSystems;

	void PrePhysicsStep(float aDeltaTime);
	void PostPhysicsStep(float aDeltaTime);

	btBroadphaseInterface* myBroadphase;
	btDefaultCollisionConfiguration* myConfiguration;
	btCollisionDispatcher* myDispatcher;
	btSequentialImpulseConstraintSolver* mySolver; // for now using default one, should try ODE quickstep solver later 
	btDiscreteDynamicsWorld* myWorld;
	btGhostPairCallback* myGhostCallback;
	
	tbb::spin_mutex myCommandsLock;
	std::atomic<bool> myIsBeingStepped;
	// TODO: should probably have per-type reusable command caches, 
	// so that even it points to heap, it'll be layed out continuously
	// - should make more cache friendly
	std::vector<const PhysicsCommand*> myCommands;

	void ResolveCommands();

private:
	friend class PhysicsEntity;
	void EnqueueCommand(const PhysicsCommand* aCmd);
	bool IsStepping() const { return myIsBeingStepped; }

#ifdef ASSERT_MUTEX
	// Assert locks to ensure safety of stepping the world
	// and related operations
	mutable AssertRWMutex mySimulationMutex;
#endif

	// all command handlers
private:
	void AddBodyHandler(const PhysicsCommandAddBody& aCmd);
	void RemoveBodyHandler(const PhysicsCommandRemoveBody& aCmd);
	void DeleteBodyHandler(const PhysicsCommandDeleteBody& aCmd);
};