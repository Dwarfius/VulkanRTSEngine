#pragma once

#include <Core/RefCounted.h>
#include <Core/Pool.h>
#include <Core/Grid.h>
#include <Core/StableVector.h>

class Game;
class GameObject;
class Model;
class Pipeline;
class Texture;
class Camera;
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

	void Update(Game& aGame, float aDeltaTime);

private:
	Handle<Texture> myGreenTankTexture;
	Handle<Texture> myRedTankTexture;
	Handle<Texture> myGreyTexture;
	Handle<Pipeline> myDefaultPipeline;
	Handle<Model> myTankModel;
	Handle<Model> mySphereModel;

	float mySpawnRate = 10.f;
	uint32_t mySpawnSquareSide = 64;
	float myTankSpeed = 5.f;
	float myShootCD = 2.f;
	float myShotLife = 10.f;
	float myShotSpeed = 10.f;

	constexpr static uint8_t kHistorySize = 100;
	float myDeltaHistory[kHistorySize]{0};
	bool myDrawShapes = false;
	bool myWipeEverything = false;
	void DrawUI(Game& aGame, float aDeltaTime);

	std::default_random_engine myRandEngine;

	constexpr static float kTankScale = 0.01f;
	float myTankAccum = 0.f;
	bool myTankSwitch;
	struct Tank
	{
		Handle<GameObject> myGO;
		glm::vec3 myDest;
		float myCooldown;
		bool myTeam;
	};
	StableVector<Tank, 1024> myTanks;
	std::shared_ptr<PhysicsShapeBox> myTankShape;
	struct TankOrBall // TODO: this is nasty :/ explore variant impact
	{
		void* myPtr;
		bool myIsTank;

		bool operator==(const TankOrBall& aOther) const
		{
			return myPtr == aOther.myPtr;
		}
	};
	Grid<TankOrBall> myGrid;

	void UpdateTanks(Game& aGame, float aDeltaTime);

	constexpr static float kBallScale = 0.2f;
	struct Ball
	{
		Handle<GameObject> myGO;
		glm::vec3 myVel;
		float myLife;
		bool myTeam;
	};
	StableVector<Ball, 1024> myBalls;
	std::shared_ptr<PhysicsShapeSphere> myBallShape;
	void UpdateBalls(Game& aGame, float aDeltaTime);

	float myRotationAngle = 0.f;
	float myFlightSpeed = 1.f;
	bool myControlCamera = false;
	void UpdateCamera(Camera& aCam, float aDeltaTime);

	PoolPtr<Light> myLight;

	void CheckCollisions(Game& aGame);
	void WipeEverything(Game& aGame);
	void CreateTerrain(Game& aGame, uint32_t aSize);
};