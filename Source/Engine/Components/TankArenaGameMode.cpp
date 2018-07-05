#include "Common.h"
#include "TankArenaGameMode.h"

#include "../Game.h"
#include "../GameObject.h"

#include "Tank.h"
#include "Renderer.h"

#include "Camera.h"

TankArenaGameMode* TankArenaGameMode::ourInstance = nullptr;

TankArenaGameMode::TankArenaGameMode()
	: myAngles(0.f)
	, mySpawnRate(75.f)
	, myAccumSpawn(0.f)
	, myAccumTime(0.f)
	, myTeamTurn(false)
{
	ourInstance = this;
}

void TankArenaGameMode::Destroy()
{
	for (GameObject *tank : myEnemyTanks)
	{
		tank->Die();
	}
	ourInstance = nullptr;
}

void TankArenaGameMode::Update(float deltaTime)
{
	const float spawnRateAccel = 0.05f;
	const int fieldSize = 80;
	const int halfSize = fieldSize / 2;

	mySpawnRate += spawnRateAccel * deltaTime;
	myAccumSpawn += deltaTime * mySpawnRate;
	if (myAccumSpawn > 1)
	{
		uint32_t spawnCount = (uint32_t)myAccumSpawn;
		myAccumSpawn -= spawnCount;

		const glm::vec3 pos = myOwner->GetTransform().GetPos();

		for (uint32_t i = 0; i < spawnCount; i++)
		{
			float side = myTeamTurn ? -1.f : 1.f;
			glm::vec3 spawnPoint = pos + glm::vec3(side * halfSize, 0, rand() % fieldSize - halfSize);
			GameObject *go = Game::GetInstance()->Instantiate(spawnPoint, glm::vec3(), glm::vec3(0.005f));
			if (go) // we might exceed game's hard limit of Game::maxObjects objects
			{
				Tank* tank = new Tank(myTeamTurn);
				tank->SetOnDeathCallback([this](ComponentBase* aComp) {
					if (ourInstance) // check if GameMode is still alive
					{
						myEnemyTanks.erase(aComp->GetOwner());
					}
				});
				spawnPoint.x *= -1.f;
				spawnPoint.z = static_cast<float>(rand() % fieldSize - halfSize);
				tank->SetNavTarget(spawnPoint);
				go->AddComponent(tank);
				go->AddComponent(new Renderer(2, 0, myTeamTurn ? 4 : 5));
				myEnemyTanks.insert(go);

				myTeamTurn = !myTeamTurn;
			}
		}
	}

	// 3rd person camera
	Camera *cam = Game::GetInstance()->GetCamera();
	Transform& camTransf = cam->GetTransform();
	const float camDist = 20;
	const float camSpeed = -0.3f;

	myAngles.y += camSpeed * deltaTime;
	glm::vec3 curPos = glm::vec3();
	glm::vec3 camPos = curPos - glm::vec3(0, 0, camDist);
	camPos = Transform::RotateAround(camPos, curPos, myAngles);
	camPos.y = camDist;

	// making sure we don't go under terrain
	camTransf.SetPos(camPos);
	camTransf.LookAt(curPos);

	myAccumTime += deltaTime;
	if (myAccumTime > 60.f)
	{
		Game::GetInstance()->EndGame();
	}
}