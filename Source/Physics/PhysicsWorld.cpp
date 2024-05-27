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
	// TODO: Bullet has a multithread solver with TBB support! Need to investigate
	// how to set it up (see BT_THREADSAFE)
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
	// Note: this contains rigidbodies as well!
	btCollisionObjectArray& collObjects = myWorld->getCollisionObjectArray();
	for (int i = 0; i < collObjects.size(); i++)
	{
		PhysicsEntity* physEntity = static_cast<PhysicsEntity*>(collObjects[i]->getUserPointer());
		physEntity->myState = PhysicsEntity::State::NotInWorld;
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

namespace
{
	struct PhysicsProfile
	{
		std::string_view myName;
		uint32_t myId;
	};
	static thread_local std::vector<PhysicsProfile> ourActiveProfile = [] {
		std::vector<PhysicsProfile> profile;
		profile.reserve(2048);
		return profile;
	} ();
}

void PhysicsWorld::EnablePhysicsProfiling()
{
	btSetCustomEnterProfileZoneFunc([](const char* aName) {
		const uint32_t index = Profiler::GetInstance().StartScope(aName);
		ourActiveProfile.push_back({ aName, index });
	});

	btSetCustomLeaveProfileZoneFunc([] {
		PhysicsProfile lastProfile = ourActiveProfile.back();
		ourActiveProfile.pop_back();
		Profiler::GetInstance().EndScope(lastProfile.myId, lastProfile.myName);
	});
}

void PhysicsWorld::PrePhysicsStep(float aDeltaTime)
{
	for (ISymCallbackListener* system : myPhysSystems)
	{
		system->OnPrePhysicsStep(aDeltaTime);
	}

	btAlignedObjectArray<btRigidBody*>& rigidbodies = myWorld->getNonStaticRigidBodies();
	for (int i = 0; i < rigidbodies.size(); i++)
	{
		PhysicsEntity* physEntity = static_cast<PhysicsEntity*>(rigidbodies[i]->getUserPointer());
		physEntity->ApplyForces();
	}

	// update all the trigger positions before simulation
	for (btCollisionObject* trigger : myTriggers)
	{
		PhysicsEntity* physEntity = static_cast<PhysicsEntity*>(trigger->getUserPointer());
		physEntity->UpdateTransform();
	}
}

void PhysicsWorld::PostPhysicsStep(float aDeltaTime)
{
	{
		std::unordered_set<uint64_t> processedPairs;
		processedPairs.reserve(myTriggers.size());
		for (btCollisionObject* collObj : myTriggers)
		{
			PhysicsEntity* triggerPhysEntity = static_cast<PhysicsEntity*>(collObj->getUserPointer());
			const uint32_t thisInd = std::bit_cast<uint32_t>(collObj->getWorldArrayIndex());

			btGhostObject* ghostObj = static_cast<btGhostObject*>(collObj);
			const int count = ghostObj->getNumOverlappingObjects();
			for (int i = 0; i < count; i++)
			{
				const btCollisionObject* other = ghostObj->getOverlappingObject(i);
				const uint32_t otherInd = std::bit_cast<uint32_t>(other->getWorldArrayIndex());

				{
					const uint32_t left = thisInd < otherInd ? thisInd : otherInd;
					const uint32_t right = thisInd < otherInd ? otherInd : thisInd;
					const uint64_t key = static_cast<uint64_t>(left) << 32 | right;
					if (processedPairs.contains(key))
					{
						continue;
					}
					processedPairs.insert(key);
				}

				PhysicsEntity* otherPhysEntity = static_cast<PhysicsEntity*>(other->getUserPointer());
				for (ISymCallbackListener* system : myPhysSystems)
				{
					system->OnTriggerCallback(*triggerPhysEntity, *otherPhysEntity);
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
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->addCollisionObject(entity->myBody, btBroadphaseProxy::SensorTrigger);
		myTriggers.push_back(entity->myBody);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->addRigidBody(static_cast<btRigidBody*>(entity->myBody));
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
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->btCollisionWorld::removeCollisionObject(entity->myBody);
		// TODO: accelerate this by using binary search
		std::erase(myTriggers, entity->myBody);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->removeRigidBody(static_cast<btRigidBody*>(entity->myBody));
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
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->btCollisionWorld::removeCollisionObject(aCmd.myOldBody);
		// TODO: accelerate this by using binary search
		std::erase(myTriggers, aCmd.myOldBody);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->removeRigidBody(static_cast<btRigidBody*>(aCmd.myOldBody));
		break;
	default:
		ASSERT(false);
	}

	switch (entity->GetType())
	{
	case PhysicsEntity::Type::Static:
		myWorld->addCollisionObject(entity->myBody);
		break;
	case PhysicsEntity::Type::Trigger:
		myWorld->addCollisionObject(entity->myBody, btBroadphaseProxy::SensorTrigger);
		myTriggers.push_back(entity->myBody);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->addRigidBody(static_cast<btRigidBody*>(entity->myBody));
		break;
	default:
		ASSERT(false);
	}

	if (aCmd.myOldType == PhysicsEntity::Type::Dynamic)
	{
		btRigidBody* rigidbody = static_cast<btRigidBody*>(aCmd.myOldBody);
		if (rigidbody->getMotionState())
		{
			delete rigidbody->getMotionState();
		}
	}
	delete aCmd.myOldBody;
}