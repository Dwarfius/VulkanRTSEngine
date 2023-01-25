#pragma once

#include <Core/RefCounted.h>
#include <Core/Pool.h>

class Game;
class GameObject;
class Model;
class Pipeline;
class Texture;
class Camera;
class PhysicsEntity;
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
	void DrawUI(Game& aGame, float aDeltaTime);

	std::default_random_engine myRandEngine;
	friend class TriggersTracker;
	TriggersTracker* myTriggersTracker;

	float myTankAccum = 0.f;
	bool myTankSwitch;
	struct Tank
	{
		Handle<GameObject> myGO;
		glm::vec3 myDest;
		float myCooldown;
		PhysicsEntity* myTrigger;
		bool myTeam;
	};
	std::vector<Tank> myTanks;
	std::vector<Tank> myTanksToRemove;
	void UpdateTanks(Game& aGame, float aDeltaTime);

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
	void UpdateBalls(Game& aGame, float aDeltaTime);

	float myRotationAngle = 0.f;
	void UpdateCamera(Camera& aCam, float aDeltaTime);

	PoolPtr<Light> myLight;
};