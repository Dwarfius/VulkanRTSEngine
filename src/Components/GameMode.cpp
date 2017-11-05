#include "Common.h"
#include "Components/GameMode.h"

#include "Game.h"
#include "GameObject.h"
#include "Camera.h"

#include "Components/Tank.h"
#include "Components/Renderer.h"

GameMode* GameMode::instance = nullptr;

GameMode::GameMode()
{
	instance = this;
}

void GameMode::Destroy()
{
	for (GameObject *tank : enemyTanks)
		tank->Die();
	instance = nullptr;
}

void GameMode::Update(float deltaTime)
{
	const int fieldSize = 80;
	const int halfSize = fieldSize / 2;

	spawnRate += spawnRateAccel * deltaTime;
	accumSpawn += deltaTime * spawnRate;
	if (accumSpawn > 1)
	{
		uint32_t spawnCount = (uint32_t)accumSpawn;
		accumSpawn -= spawnCount;

		vec3 pos = owner->GetTransform()->GetPos();

		for (uint32_t i = 0; i < spawnCount; i++)
		{
			float side = teamTurn ? -1 : 1;
			vec3 spawnPoint = pos + vec3(side * halfSize, 0, rand() % fieldSize - halfSize);
			GameObject *go = Game::GetInstance()->Instantiate(spawnPoint, vec3(), vec3(0.005f));
			if (go) // we might exceed game's hard limit of Game::maxObjects objects
			{
				Tank *tank = new Tank(teamTurn);
				tank->SetOnDeathCallback([this](ComponentBase* comp) {
					if(instance) // check if GameMode is still alive
						enemyTanks.erase(comp->GetOwner());
				});
				spawnPoint.x *= -1;
				spawnPoint.z = rand() % fieldSize - halfSize;
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
	Transform *camTransf = cam->GetTransform();
	const float camDist = 20;
	const float camSpeed = -0.3f;

	angles += vec3(0, camSpeed, 0) * deltaTime;
	vec3 curPos = vec3();
	vec3 camPos = curPos - vec3(0, 0, camDist);
	camPos = Transform::RotateAround(camPos, curPos, angles);
	camPos.y = camDist;

	// making sure we don't go under terrain
	camTransf->SetPos(camPos);
	camTransf->LookAt(curPos);

	accumTime += deltaTime;
	if (accumTime > 60)
		Game::GetInstance()->EndGame();
}