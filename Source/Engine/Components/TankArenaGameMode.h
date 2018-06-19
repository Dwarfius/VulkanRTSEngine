#pragma once

#include "ComponentBase.h"

class GameObject;

class TankArenaGameMode : public ComponentBase
{
public:
	TankArenaGameMode();

	void Update(float aDeltaTime) override;
	void Destroy() override;

	static TankArenaGameMode* GetInstance() { return ourInstance; }

private:
	static TankArenaGameMode* ourInstance;

	glm::vec3 myAngles;
	float mySpawnRate;
	float myAccumSpawn;
	float myAccumTime;

	bool myTeamTurn;

	// access to this is consecutive, no need for TBB
	unordered_set<GameObject*> myEnemyTanks;
};