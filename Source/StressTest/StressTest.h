#pragma once

#include <Core/RefCounted.h>

class Game;
class GameObject;
class Model;
class Pipeline;
class Texture;
class Camera;

// Sets up and runs the Tank stress test:
// a "wall" of tanks descends towards the middle,
// shooting each other.
class StressTest
{
public:
	StressTest(Game& aGame);

	void Update(Game& aGame, float aDeltaTime);

private:
	Handle<Texture> myTankTexture;
	Handle<Pipeline> myTankPipeline;
	Handle<Model> myTankModel;

	float myTankLife = 10.f;
	float mySpawnRate = 10.f;
	float mySpawnSquareSide = 20.f;
	void DrawUI(Game& aGame);

	struct Tank
	{
		Handle<GameObject> myGO;
		float myLife;
	};
	std::vector<Tank> myTanks;
	float myTankAccum = 0.f;
	std::default_random_engine myRandEngine;
	void UpdateTanks(Game& aGame, float aDeltaTime);

	float myRotationAngle = 0.f;
	void UpdateCamera(Camera& aCam, float aDeltaTime);
};