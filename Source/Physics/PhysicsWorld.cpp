#include "Precomp.h"
#include "PhysicsWorld.h"

#include "PhysicsDebugDrawer.h"
#include "PhysicsEntity.h"

#include <Core/Utils.h>

#include <BulletCollision/CollisionShapes/btTriangleShape.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

PhysicsWorld::PhysicsWorld()
	: myIsBeingStepped(false)
{
	myBroadphase = new btDbvtBroadphase();
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
	myWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawContactPoints);

	myCommands.reserve(200);
	myEntities.reserve(1000);
}

PhysicsWorld::~PhysicsWorld()
{
	// first gonna pump through the delete/remove requests
	// this will clear up the allocated requests and get rid
	// of bodies that game doesn't need
	ResolveCommands();

	// everything that's left can be marked as outside of world
	// if they want to be deleted later
	for (PhysicsEntity* entity : myEntities)
	{
		entity->myState = PhysicsEntity::State::NotInWorld;
	}

	delete myWorld->getDebugDrawer();
	delete myWorld;
	delete mySolver;
	delete myDispatcher;
	delete myConfiguration;
	delete myBroadphase;
}

void PhysicsWorld::AddEntity(PhysicsEntity* anEntity)
{
	ASSERT(anEntity->GetState() == PhysicsEntity::State::NotInWorld);
	ASSERT(!anEntity->myWorld);
	anEntity->myState = PhysicsEntity::State::PendingAddition;
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
	ASSERT(anEntity->GetState() == PhysicsEntity::State::InWorld);
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

		myIsBeingStepped = true;
		// even if we don't have enough deltaTime this frame, Bullet will avoid stepping
		// the simulation, but it will update the motion states, thus achieving interpolation
		myWorld->stepSimulation(aDeltaTime, MaxSteps, FixedStepLength);
		myIsBeingStepped = false;

		myWorld->debugDrawWorld();
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

	// TODO: have a separate container for dynamic entities for faster iter
	for (PhysicsEntity* entity : myEntities)
	{
		if (entity->GetType() == PhysicsEntity::Type::Dynamic)
		{
			entity->ApplyForces();
		}
	}
}

void PhysicsWorld::PostPhysicsStep(float aDeltaTime)
{
	for (ISymCallbackListener* system : myPhysSystems)
	{
		system->OnPostPhysicsStep(aDeltaTime);
	}
}

// little macro helper for switch cases to reduce the length
#define CALL_COMMAND_HANDLER(Type, Cmd) case PhysicsCommand::Type: Type##Handler (static_cast<const PhysicsCommand##Type&>(Cmd)); break;
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
			CALL_COMMAND_HANDLER(DeleteBody, cmdRef);
			default: ASSERT(false);
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
	PhysicsEntity* entity = aCmd.myEntity;

	ASSERT(!entity->myWorld);
	ASSERT(entity->GetState() == PhysicsEntity::State::PendingAddition);

	switch (entity->GetType())
	{
	case PhysicsEntity::Type::Static:
	case PhysicsEntity::Type::Trigger:
		myWorld->addCollisionObject(entity->myBody);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->addRigidBody(static_cast<btRigidBody*>(entity->myBody));
		break;
	default:
		ASSERT(false);
	}
	entity->myWorld = this;
	entity->myState = PhysicsEntity::State::InWorld;
	myEntities.push_back(entity);
}

void PhysicsWorld::RemoveBodyHandler(const PhysicsCommandRemoveBody& aCmd)
{
	PhysicsEntity* entity = aCmd.myEntity;

	ASSERT(entity->myWorld == this);
	ASSERT(entity->GetState() == PhysicsEntity::State::PendingRemoval);

	switch (entity->GetType())
	{
	case PhysicsEntity::Type::Static:
	case PhysicsEntity::Type::Trigger:
		myWorld->removeCollisionObject(entity->myBody);
		break;
	case PhysicsEntity::Type::Dynamic:
		myWorld->removeRigidBody(static_cast<btRigidBody*>(entity->myBody));
		break;
	default:
		ASSERT(false);
	}
	entity->myWorld = nullptr;
	entity->myState = PhysicsEntity::State::NotInWorld;

	// TODO: get rid of this by using u_map
	size_t size = myEntities.size();
	for (size_t i = 0; i < size; i++)
	{
		if (myEntities[i] == entity)
		{
			myEntities.erase(myEntities.begin() + i);
			break;
		}
	}
}

void PhysicsWorld::DeleteBodyHandler(const PhysicsCommandDeleteBody& aCmd)
{
	// remove it from the world first
	PhysicsCommandRemoveBody remCmd(aCmd.myEntity);
	RemoveBodyHandler(remCmd);

	// now it's safe to delete it
	delete aCmd.myEntity;
}