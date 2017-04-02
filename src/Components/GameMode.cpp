#include "Components\GameMode.h"
#include "Game.h"
#include "Components\Tank.h"

GameMode* GameMode::instance = nullptr;

GameMode::GameMode()
{
	instance = this;
}

void GameMode::Destroy()
{
	instance = nullptr;
}

void GameMode::Update(float deltaTime)
{
	spawnRate += spawnRateAccel * deltaTime;
	accumSpawn += deltaTime * spawnRate;
	if (accumSpawn > 1)
	{
		uint32_t spawnCount = (uint32_t)accumSpawn;
		accumSpawn -= spawnCount;

		vec3 pos = owner->GetTransform()->GetPos();

		for (uint32_t i = 0; i < spawnCount; i++)
		{
			float x = (rand() % 100 - 50) / 10.f;
			float z = (rand() % 100 - 50) / 10.f;
			vec3 dir = normalize(vec3(x, 0, z));
			vec3 spawnPoint = pos + dir * spawnRadius;
			GameObject *go = Game::GetInstance()->Instantiate(spawnPoint, vec3(), vec3(0.005f));
			if (go) // we might exceed game's hard limit of Game::maxObjects objects
			{
				Tank *tank = new Tank();
				tank->SetOnDeathCallback([this](ComponentBase* comp) {
					if(instance) // check if GameMode is still alive
						enemyTanks.erase(comp->GetOwner());
				});
				go->AddComponent(tank);
				go->AddComponent(new Renderer(2, 0, 5));
				enemyTanks.insert(go);
			}
		}
	}
}