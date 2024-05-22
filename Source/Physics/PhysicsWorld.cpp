#include "Precomp.h"
#include "PhysicsWorld.h"

#include "PhysicsCommands.h"
#include "PhysicsDebugDrawer.h"
#include "PhysicsEntity.h"

#include <Core/Utils.h>
#include <Core/Profiler.h>

#include <BulletCollision/CollisionShapes/btTriangleShape.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

PhysicsWorld::PhysicsWorld()
	: myIsBeingStepped(false)
{
	myBroadphase = new btDbvtBroadphase();
	myGhostCallback = new btGhostPairCallback();
	myBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(myGhostCallback);
	myConfiguration = new btDefaultCollisionConfiguration();
	myDispatcher = new btCollisionDispatcher(myConfiguration);
	mySolver = new btSequentialImpulseConstraintSolver();
	myWorld = new btDiscreteDynamicsWorld(myDispatcher, myBroadphase, mySolver, myConfiguration);
	
	// Setting up pre physics step callback
	myWorld->setInternalTickCallback([](btDynamicsWorld* aWorld, float aDeltaTime) {
		PhysicsWorld* physWorld = static_cast<PhysicsWorld*>(aWorld->getWorldUserInfo());
		physWorld->PrePhysicsStep(aDeltaTime);
	}, this, true);
	// Setting up post physics step callback
	myWorld->setInternalTickCallback([](btDynamicsWorld* aWorld, float aDeltaTime) {
		PhysicsWorld* physWorld = static_cast<PhysicsWorld*>(aWorld->getWorldUserInfo());
		physWorld->PostPhysicsStep(aDeltaTime);
	}, this, false);

	myWorld->setDebugDrawer(new PhysicsDebugDrawer());

	myCommands.reserve(200);
	myStaticEntities.reserve(1000);
	myDynamicEntities.reserve(200);
	myTriggers.reserve(100);
}

PhysicsWorld::~PhysicsWorld()
{
	// first gonna pump through the delete/remove requests
	// this will clear up the allocated requests and get rid
	// of bodies that game doesn't need
	ResolveCommands();

	// everything that's left can be marked as outside of world
	// if they want to be deleted later
	for (PhysicsEntity* entity : myStaticEntities)
	{
		entity->myState = PhysicsEntity::State::NotInWorld;
	}

	for (PhysicsEntity* entity : myDynamicEntities)
	{
		entity->myState = PhysicsEntity::State::NotInWorld;
	}

	for (PhysicsEntity* entity : myTriggers)
	{
		entity->myState = PhysicsEntity::State::NotInWorld;
	}

	delete myWorld->getDebugDrawer();
	delete myWorld;
	delete mySolver;
	delete myDispatcher;
	delete myConfiguration;
	delete myGhostCallback;
	delete myBroadphase;
}

void PhysicsWorld::AddEntity(PhysicsEntity* anEntity)
{
	ASSERT(anEntity->GetState() == PhysicsEntity::State::NotInWorld);
	ASSERT(!anEntity->myWorld);
	anEntity->myState = PhysicsEntity::State::PendingAddition;
	anEntity->myWorld = this;
	// TODO: get rid of allocs/deallocs by using an internal recycler
	const PhysicsCommandAddBody* cmd = new PhysicsCommandAddBody(anEntity);
	EnqueueCommand(cmd);
}

void PhysicsWorld::RemoveEntity(PhysicsEntity* anEntity)
{
	ASSERT(anEntity->GetState() == PhysicsEntity::State::InWorld);
	ASSERT(anEntity->myWorld == this);
	anEntity->myState = PhysicsEntity::State::PendingRemoval;
	// TODO: get rid of allocs/deallocs by using an internal recycler
	const PhysicsCommandRemoveBody* cmd = new PhysicsCommandRemoveBody(anEntity);
	EnqueueCommand(cmd);
}

void PhysicsWorld::DeleteEntity(PhysicsEntity* anEntity)
{
	ASSERT(anEntity->GetState() == PhysicsEntity::State::InWorld
		|| anEntity->GetState() == PhysicsEntity::State::PendingAddition);
	ASSERT(anEntity->myWorld == this);
	anEntity->myState = PhysicsEntity::State::PendingRemoval;
	// TODO: get rid of allocs/deallocs by using an internal recycler
	const PhysicsCommandDeleteBody* cmd = new PhysicsCommandDeleteBody(anEntity);
	EnqueueCommand(cmd);
}

void PhysicsWorld::Simulate(float aDeltaTime)
{
	ResolveCommands();

	{
#ifdef ASSERT_MUTEX
		AssertWriteLock writeLock(mySimulationMutex);
#endif

		{
			Profiler::ScopedMark scope("PhysicsWorld::StepSimulation");
			myIsBeingStepped = true;
			// even if we don't have enough deltaTime this frame, Bullet will avoid stepping
			// the simulation, but it will update the motion states, thus achieving interpolation
			myWorld->stepSimulation(aDeltaTime, kMaxSteps, kFixedStepLength);
			myIsBeingStepped = false;
		}

		{
			Profiler::ScopedMark scope("PhysicsWorld::DebugDraw");
			myWorld->debugDrawWorld();
		}
	}
}

bool PhysicsWorld::RaycastClosest(glm::vec3 aFrom, glm::vec3 aDir, float aDist, PhysicsEntity*& aHitEntity) const
{
	glm::vec3 to = aFrom + aDir * aDist;
	return RaycastClosest(aFrom, to, aHitEntity);
}

bool PhysicsWorld::RaycastClosest(glm::vec3 aFrom, glm::vec3 aTo, PhysicsEntity*& aHitEntity) const
{
	const btVector3 from = Utils::ConvertToBullet(aFrom);
	const btVector3 to = Utils::ConvertToBullet(aTo);

	btCollisionWorld::ClosestRayResultCallback closestCollector(from, to);
	// by defalt we don't want hits with backface triangles
	closestCollector.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
	{
#ifdef ASSERT_MUTEX
		AssertReadLock readLock(mySimulationMutex);
#endif
		myWorld->rayTest(from, to, closestCollector);
	}

	if (closestCollector.hasHit())
	{
		aHitEntity = static_cast<PhysicsEntity*>(closestCollector.m_collisionObject->getUserPointer());
		return true;
	}
	return false;
}

bool PhysicsWorld::Raycast(glm::vec3 aFrom, glm::vec3 aDir, float aDist, std::vector<PhysicsEntity*>& anAllHits) const
{
	glm::vec3 to = aFrom + aDir * aDist;
	return Raycast(aFrom, to, anAllHits);
}

bool PhysicsWorld::Raycast(glm::vec3 aFrom, glm::vec3 aTo, std::vector<PhysicsEntity*>& anAllHits) const
{
	const btVector3 from = Utils::ConvertToBullet(aFrom);
	const btVector3 to = Utils::ConvertToBullet(aTo);

	btCollisionWorld::AllHitsRayResultCallback allCollector(from, to);
	// by defalt we don't want hits with backface triangles
	allCollector.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
	{
#ifdef ASSERT_MUTEX
		AssertReadLock readLock(mySimulationMutex);
#endif
		myWorld->rayTest(from, to, allCollector);
	}
	if (allCollector.hasHit())
	{
		const btAlignedObjectArray<const btCollisionObject*>& hits = allCollector.m_collisionObjects;
		const int hitCount = hits.size();
		anAllHits.resize(hitCount);
		for (int i = 0; i < hitCount; i++)
		{
			anAllHits[i] = static_cast<PhysicsEntity*>(hits[i]->getUserPointer());
		}
		return true;
	}
	return false;
}

const DebugDrawer& PhysicsWorld::GetDebugDrawer() const
{
	return static_cast<PhysicsDebugDrawer*>(myWorld->getDebugDrawer())->GetDebugDrawer();
}

bool PhysicsWorld::IsDebugDrawingEnabled() const
{
	return myWorld->getDebugDrawer()->getDebugMode() != btIDebugDraw::DBG_NoDebug;
}

void PhysicsWorld::SetDebugDrawing(bool aIsEnabled)
{
	const int debugMode = aIsEnabled
		? btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawContactPoints
		: btIDebugDraw::DBG_NoDebug;
	myWorld->getDebugDrawer()->setDebugMode(debugMode);
}

void PhysicsWorld::AddPhysSystem(ISymCallbackListener* aSystem)
{
	myPhysSystems.push_back(aSystem);
}

void PhysicsWorld::PrePhysicsStep(float aDeltaTime)
{
	for (ISymCallbackListener* system : myPhysSystems)
	{
		system->OnPrePhysicsStep(aDeltaTime);
	}

	for (PhysicsEntity* entity : myDynamicEntities)
	{
		entity->ApplyForces();
	}

	// update all teh trigger positions before simulation
	for (PhysicsEntity* trigger : myTriggers)
	{
		trigger->UpdateTransform();
	}
}

struct EntityPair
{
	const PhysicsEntity* myLeft;
	const PhysicsEntity* myRight;

	auto operator<=>(const EntityPair&) const = default;
};

namespace std
{
	template<> struct hash<EntityPair>
	{
		size_t operator()(const EntityPair& aPair) const
		{
			return hash<const void*>()(aPair.myLeft) ^
				hash<const void*>()(aPair.myRight) << 1;
		}
	};
}

void PhysicsWorld::PostPhysicsStep(float aDeltaTime)
{
	{
		std::unordered_set<EntityPair> processedPairs;
		processedPairs.reserve(myTriggers.size());
		for (PhysicsEntity* trigger : myTriggers)
		{
			int count = trigger->GetOverlapCount();
			for (int i = 0; i < count; i++)
			{
				const PhysicsEntity* other = trigger->GetOverlapContact(i);

				EntityPair pair;
				pair.myLeft = trigger < other ? trigger : other;
				pair.myRight = trigger < other ? other : trigger;
				if (processedPairs.contains(pair))
				{
					continue;
				}
				processedPairs.insert(pair);

				for (ISymCallbackListener* system : myPhysSystems)
				{
					system->OnTriggerCallback(*trigger, *other);
				}
			}
		}
	}

	for (ISymCallbackListener* system : myPhysSystems)
	{
		system->OnPostPhysicsStep(aDeltaTime);
	}
}

// little macro helper for switch cases to reduce the length
void PhysicsWorld::ResolveCommands()
{
	Profiler::ScopedMark scope("PhysicsWorld::ResolveCommands");
	tbb::spin_mutex::scoped_lock lock(myCommandsLock);
	std::unordered_set<PhysicsEntity*> skippedPhysEntities;
	for (const PhysicsCommand* cmd : myCommands)
	{
		const PhysicsCommand& cmdRef = *cmd;
		switch (cmdRef.myType)
		{
		case PhysicsCommand::AddBody:
			AddBodyHandler(static_cast<const PhysicsCommandAddBody&>(cmdRef), skippedPhysEntities);
			break;
		case PhysicsCommand::RemoveBody:
			RemoveBodyHandler(static_cast<const PhysicsCommandRemoveBody&>(cmdRef), skippedPhysEntities);
			break;
		case PhysicsCommand::DeleteBody:
			DeleteBodyHandler(static_cast<const PhysicsCommandDeleteBody&>(cmdRef), skippedPhysEntities);
			break;
		case PhysicsCommand::ChangeBody:
			ChangeBodyHandler(static_cast<const PhysicsCommandChangeBody&>(cmdRef));
			break;
		default:
			ASSERT(false);
			break;
		}
		// TODO: get rid of allocs/deallocs by using an internal recycler
		delete cmd;
	}
	myCommands.clear();
}

void PhysicsWorld::EnqueueCommand(const PhysicsCommand* aCmd)
{
	tbb::spin_mutex::scoped_lock lock(myCommandsLock);
	myCommands.push_back(aCmd);
}

void PhysicsWorld::AddBodyHandler(const PhysicsCommandAddBody& aCmd, std::unordered_set<PhysicsEntity*>& aSkippedSet)
{
	PhysicsEntity* entity = aCmd.myEntity;
	ASSERT(entity->myWorld == this);

	// It's possible the entity got requested to be added right as we're shutting down the world
	if (entity->GetState() == PhysicsEntity::State::PendingRemoval)
	{
		aSkippedSet.insert(entity);
		return;
	}

	ASSERT(entity->GetState() == PhysicsEntity::State::PendingAddition);

	switch (entity->GetType())
	{
	case PhysicsEntity::Type::Static:
		myWorld->addCollisionObject(entity->myBody);
		myStaticEntities.push_back(entity);
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->addCollisionObject(entity->myBody, btBroadphaseProxy::SensorTrigger);
		myTriggers.push_back(entity);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->addRigidBody(static_cast<btRigidBody*>(entity->myBody));
		myDynamicEntities.push_back(entity);
		break;
	default:
		ASSERT(false);
	}
	entity->myState = PhysicsEntity::State::InWorld;
}

void PhysicsWorld::RemoveBodyHandler(const PhysicsCommandRemoveBody& aCmd, const std::unordered_set<PhysicsEntity*>& aSkippedSet)
{
	PhysicsEntity* entity = aCmd.myEntity;

	ASSERT(entity->myWorld == this);
	ASSERT(entity->GetState() == PhysicsEntity::State::PendingRemoval);

	entity->myWorld = nullptr;
	entity->myState = PhysicsEntity::State::NotInWorld;

	if (aSkippedSet.contains(entity))
	{
		// Nothing to do, since it's addition was skipped
		return;
	}

	switch (entity->GetType())
	{
	case PhysicsEntity::Type::Static:
		myWorld->btCollisionWorld::removeCollisionObject(entity->myBody);
		// TODO: accelerate this by using binary search
		std::erase(myStaticEntities, entity);
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->btCollisionWorld::removeCollisionObject(entity->myBody);
		std::erase(myTriggers, entity);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->removeRigidBody(static_cast<btRigidBody*>(entity->myBody));
		std::erase(myDynamicEntities, entity);
		break;
	default:
		ASSERT(false);
	}
}

void PhysicsWorld::DeleteBodyHandler(const PhysicsCommandDeleteBody& aCmd, const std::unordered_set<PhysicsEntity*>& aSkippedSet)
{
	// remove it from the world first
	PhysicsCommandRemoveBody remCmd(aCmd.myEntity);
	RemoveBodyHandler(remCmd, aSkippedSet);

	// now it's safe to delete it
	delete aCmd.myEntity;
}

void PhysicsWorld::ChangeBodyHandler(const PhysicsCommandChangeBody& aCmd)
{
	PhysicsEntity* entity = aCmd.myEntity;
	switch (aCmd.myOldType)
	{
	case PhysicsEntity::Type::Static:
		myWorld->btCollisionWorld::removeCollisionObject(aCmd.myOldBody);
		// TODO: accelerate this by using binary search
		std::erase(myStaticEntities, entity);
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->btCollisionWorld::removeCollisionObject(aCmd.myOldBody);
		std::erase(myTriggers, entity);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->removeRigidBody(static_cast<btRigidBody*>(aCmd.myOldBody));
		std::erase(myDynamicEntities, entity);
		break;
	default:
		ASSERT(false);
	}

	switch (entity->GetType())
	{
	case PhysicsEntity::Type::Static:
		myWorld->addCollisionObject(entity->myBody);
		myStaticEntities.push_back(entity);
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->addCollisionObject(entity->myBody, btBroadphaseProxy::SensorTrigger);
		myTriggers.push_back(entity);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->addRigidBody(static_cast<btRigidBody*>(entity->myBody));
		myDynamicEntities.push_back(entity);
		break;
	default:
		ASSERT(false);
	}

	delete aCmd.myOldBody;
}