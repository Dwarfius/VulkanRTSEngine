#include "Components\GameMode.h"
#include "Game.h"
#include "Components\Tank.h"

GameMode* GameMode::instance = nullptr;

GameMode::GameMode()
{
	instance = this;
	string msg = "\nWelcome to this small arcade survival game. The goal is to survive as long as possible";
	msg += "and to destroy as many tanks as possible. Controls are WSAD to move, mouse to aim and shoot";
	msg += "(hold to charge), U and J are volume controls, I and K change mouse sensitivity. Good luck!\n";
	printf(msg.c_str());
}

void GameMode::Destroy()
{
	printf("You're dead - final score: %d\n", score);
	printf("Press R to restart\n");
	for (GameObject *tank : enemyTanks)
		tank->Die();
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