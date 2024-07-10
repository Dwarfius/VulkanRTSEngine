#pragma once

// TODO: This replaces Bullet's broadphase logic with our own QuadTree + game resolution.
// The results aren't improving yet (moving is fast, testing is too slow).
// Sidenote, I just discovered btDbvtBroadphase has m_deferedcollide, which disables 
// collision checks during AABB move - that'll help reduce Bullet overhead, I should test it.
//#define ST_QTREE

#include <Core/RefCounted.h>
#include <Core/Pool.h>
#ifdef ST_QTREE
#include <Core/QuadTree.h>
#include <Core/StableVector.h>
#endif

class Game;
class GameObject;
class Model;
class Pipeline;
class Texture;
class Camera;
class PhysicsEntity;
class PhysicsShapeBox;
class PhysicsShapeSphere;
struct Light;

// Sets up and runs the Tank stress test:
// a "wall" of tanks descends towards the middle,
// shooting each other.
class StressTest
{
public:
	StressTest(Game& aGame);
	~StressTest();

	void Update(Game& aGame, float aDeltaTime);

private:
	Handle<Texture> myGreenTankTexture;
	Handle<Texture> myRedTankTexture;
	Handle<Texture> myGreyTexture;
	Handle<Pipeline> myDefaultPipeline;
	Handle<Model> myTankModel;
	Handle<Model> mySphereModel;

	float mySpawnRate = 10.f;
	float mySpawnSquareSide = 20.f;
	float myTankSpeed = 5.f;
	float myShootCD = 2.f;
	float myShotLife = 10.f;
	float myShotSpeed = 10.f;

	constexpr static uint8_t kHistorySize = 100;
	float myDeltaHistory[kHistorySize]{0};
	bool myDrawShapes = false;
	bool myDrawQuadTree = false;
	void DrawUI(Game& aGame, float aDeltaTime);

	std::default_random_engine myRandEngine;
	friend class TriggersTracker;
	TriggersTracker* myTriggersTracker;

	constexpr static float kTankScale = 0.01f;
	float myTankAccum = 0.f;
	bool myTankSwitch;
	struct Tank
	{
		Handle<GameObject> myGO;
		glm::vec3 myDest;
		float myCooldown;
#ifdef ST_QTREE
		QuadTree<Tank*>::Info myTreeInfo;
#else
		PhysicsEntity* myTrigger;
#endif
		bool myTeam;
	};
#ifdef ST_QTREE
	StableVector<Tank, 1024> myTanks;
#else
	std::vector<Tank> myTanks;
#endif
	std::vector<Tank> myTanksToRemove;
	std::shared_ptr<PhysicsShapeBox> myTankShape;
#ifdef ST_QTREE
	QuadTree<Tank*> myTanksTree;
#endif
	void UpdateTanks(Game& aGame, float aDeltaTime);

	constexpr static float kBallScale = 0.2f;
	struct Ball
	{
		Handle<GameObject> myGO;
		glm::vec3 myVel;
		float myLife;
		PhysicsEntity* myTrigger;
		bool myTeam;
	};
	std::vector<Ball> myBalls;
	std::vector<Ball> myBallsToRemove;
	std::shared_ptr<PhysicsShapeSphere> myBallShape;
	void UpdateBalls(Game& aGame, float aDeltaTime);

	float myRotationAngle = 0.f;
	void UpdateCamera(Camera& aCam, float aDeltaTime);

	PoolPtr<Light> myLight;
};