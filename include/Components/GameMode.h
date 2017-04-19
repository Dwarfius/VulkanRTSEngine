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

private:
	static GameMode *instance;

	const float spawnRateAccel = 0.05f;

	float spawnRate = 75.f;
	float accumSpawn = 0;
	float accumTime = 0;

	bool teamTurn;

	vec3 angles;

	// access to this is consecutive, no need for TBB
	unordered_set<GameObject*> enemyTanks;
};

#endif