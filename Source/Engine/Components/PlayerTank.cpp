#include "Common.h"
#include "PlayerTank.h"

#include "../Game.h"
#include "../Input.h"
#include "../Audio.h"
#include "../GameObject.h"
#include "../Camera.h"
#include "../Terrain.h"

#include "Tank.h"
#include "Missile.h"
#include "Renderer.h"

PlayerTank::PlayerTank()
	: myShootTimer(0.f)
	, myHeightOffset(0.f)
	, myAngles(0.f)
	, myHoldStart(-1.f)
{
}

void PlayerTank::Update(float aDeltaTime)
{
	// just general settings
	const float myShootRate = 1;
	const float speed = 2.5f;
	const float rotSpeed = 60.f;

	Camera *cam = Game::GetInstance()->GetCamera();
	Transform& camTransf = cam->GetTransform();
	Transform& ownTransf = myOwner->GetTransform();

	// update the camera
	glm::vec3 forward = ownTransf.GetForward();
	// move the gameobject
	if (Input::GetKey('W'))
	{
		ownTransf.Translate(forward * aDeltaTime * speed);
	}
	if (Input::GetKey('S'))
	{
		ownTransf.Translate(-forward * aDeltaTime * speed);
	}
	if (Input::GetKey('D'))
	{
		ownTransf.Rotate(glm::vec3(0, -rotSpeed * aDeltaTime, 0));
	}
	if (Input::GetKey('A'))
	{
		ownTransf.Rotate(glm::vec3(0, rotSpeed * aDeltaTime, 0));
	}
	
	// terrain walking
	float yOffset = myOwner->GetCenter().y * ownTransf.GetScale().y;
	const Terrain *terrain = Game::GetInstance()->GetTerrain(ownTransf.GetPos());
	glm::vec3 curPos = ownTransf.GetPos();
	curPos.y = terrain->GetHeight(curPos) + yOffset;
	ownTransf.SetPos(curPos);

	// terrain facing
	glm::vec3 newUp = terrain->GetNormal(curPos);
	ownTransf.RotateToUp(newUp);

	// 3rd person camera
	const float mouseSensitivity = 0.3f;
	glm::vec2 deltaPos = Input::GetMouseDelta() * mouseSensitivity;
	const float camDist = 3;

	myAngles += glm::vec3(deltaPos.y, -deltaPos.x, 0) * aDeltaTime;
	glm::vec3 camPos = curPos - glm::vec3(0, 0, camDist);
	camPos = Transform::RotateAround(camPos, curPos, myAngles);

	// making sure we don't go under terrain
	float heightAtCamPos = terrain->GetHeight(camPos);
	if (camPos.y < heightAtCamPos)
	{
		camPos.y = heightAtCamPos + 0.1f;
	}

	camTransf.SetPos(camPos);
	camTransf.LookAt(ownTransf.GetPos());

	if (myShootTimer > 0.f)
	{
		myShootTimer -= aDeltaTime;
	}

	if (myShootTimer <= 0.f)
	{
		if (myHoldStart == -1.f && Input::GetMouseBtn(0))
		{
			myHoldStart = Game::GetInstance()->GetTime();
		}

		if (myHoldStart >= 0.f && !Input::GetMouseBtn(0))
		{
			const float holdLength = glm::min(Game::GetInstance()->GetTime() - myHoldStart, 2.f);
			const float force = glm::mix(5.f, 20.f, holdLength / 2.f);

			// aprox pos of the cannon exit point
			const glm::vec3 spawnPos = curPos + forward * 0.6f + glm::vec3(0, 0.25f, 0);
			GameObject *go = Game::GetInstance()->Instantiate(spawnPos, glm::vec3(), glm::vec3(0.1f));
			go->AddComponent(new Renderer(3, 0, 6));
			const glm::vec3 shootDir = forward + glm::vec3(0, 0.2f, 0);
			go->AddComponent(new Missile(shootDir * force, myOwner, true));

			//Audio::Play(1, curPos);

			myShootTimer = myShootRate;
			myHoldStart = -1.f;
		}
	}
}