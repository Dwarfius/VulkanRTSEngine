#pragma once

#include "ComponentBase.h"

class GameObject;

class TankArenaGameMode : public ComponentBase
{
public:
	TankArenaGameMode();

	void Update(float deltaTime) override;
	void Destroy() override;

	static TankArenaGameMode* GetInstance() { return instance; }

private:
	static TankArenaGameMode *instance;

	const float spawnRateAccel = 0.05f;

	float spawnRate = 75.f;
	float accumSpawn = 0;
	float accumTime = 0;

	bool teamTurn;

	glm::vec3 angles;

	// access to this is consecutive, no need for TBB
	unordered_set<GameObject*> enemyTanks;
};