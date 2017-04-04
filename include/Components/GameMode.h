#ifndef _GAME_MODE_H
#define _GAME_MODE_H

#include "Components\ComponentBase.h"
#include <unordered_set>
#include "Transform.h"

using namespace std;

class GameObject;

class GameMode : public ComponentBase
{
public:
	GameMode();

	void Update(float deltaTime) override;
	void Destroy() override;

	static GameMode* GetInstance() { return instance; }

	void IncreaseScore() { ++score; } // this isn't thread safe but we don't really care, chance of occurance is too small
	int GetScore() { return score; }

private:
	static GameMode *instance;

	const float spawnRateAccel = 0.01f;

	int score = 0;

	float spawnRadius = 20;
	float spawnRate = 0.5;
	float accumSpawn = 0;

	// access to this is consecutive, no need for TBB
	unordered_set<GameObject*> enemyTanks;
};

#endif