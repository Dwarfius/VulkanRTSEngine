#include "Common.h"
#include "TankArenaGameMode.h"

#include "../Game.h"
#include "../GameObject.h"
#include "../Camera.h"

#include "Tank.h"
#include "Renderer.h"

TankArenaGameMode* TankArenaGameMode::instance = nullptr;

TankArenaGameMode::TankArenaGameMode()
{
	instance = this;
}

void TankArenaGameMode::Destroy()
{
	for (GameObject *tank : enemyTanks)
	{
		tank->Die();
	}
	instance = nullptr;
}

void TankArenaGameMode::Update(float deltaTime)
{
	const int fieldSize = 80;
	const int halfSize = fieldSize / 2;

	spawnRate += spawnRateAccel * deltaTime;
	accumSpawn += deltaTime * spawnRate;
	if (accumSpawn > 1)
	{
		uint32_t spawnCount = (uint32_t)accumSpawn;
		accumSpawn -= spawnCount;

		const glm::vec3 pos = owner->GetTransform().GetPos();

		for (uint32_t i = 0; i < spawnCount; i++)
		{
			float side = teamTurn ? -1.f : 1.f;
			glm::vec3 spawnPoint = pos + glm::vec3(side * halfSize, 0, rand() % fieldSize - halfSize);
			GameObject *go = Game::GetInstance()->Instantiate(spawnPoint, glm::vec3(), glm::vec3(0.005f));
			if (go) // we might exceed game's hard limit of Game::maxObjects objects
			{
				Tank *tank = new Tank(teamTurn);
				tank->SetOnDeathCallback([this](ComponentBase* comp) {
					if (instance) // check if GameMode is still alive
					{
						enemyTanks.erase(comp->GetOwner());
					}
				});
				spawnPoint.x *= -1;
				spawnPoint.z = static_cast<float>(rand() % fieldSize - halfSize);
				tank->SetNavTarget(spawnPoint);
				go->AddComponent(tank);
				go->AddComponent(new Renderer(2, 0, teamTurn ? 4 : 5));
				enemyTanks.insert(go);

				teamTurn = !teamTurn;
			}
		}
	}

	// 3rd person camera
	Camera *cam = Game::GetInstance()->GetCamera();
	Transform& camTransf = cam->GetTransform();
	const float camDist = 20;
	const float camSpeed = -0.3f;

	angles.y += camSpeed * deltaTime;
	glm::vec3 curPos = glm::vec3();
	glm::vec3 camPos = curPos - glm::vec3(0, 0, camDist);
	camPos = Transform::RotateAround(camPos, curPos, angles);
	camPos.y = camDist;

	// making sure we don't go under terrain
	camTransf.SetPos(camPos);
	camTransf.LookAt(curPos);

	accumTime += deltaTime;
	if (accumTime > 60)
	{
		Game::GetInstance()->EndGame();
	}
}