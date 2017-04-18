#include "Components\PlayerTank.h"
#include "Game.h"
#include "Input.h"
#include "Components\Tank.h"
#include "Components\Missile.h"
#include "Audio.h"

void PlayerTank::Update(float deltaTime)
{
	// just general settings
	const float speed = 2.5f;
	const float rotSpeed = 60.f;

	Camera *cam = Game::GetInstance()->GetCamera();
	Transform *camTransf = cam->GetTransform();
	Transform *ownTransf = owner->GetTransform();

	// update the camera
	vec3 forward = ownTransf->GetForward();
	// move the gameobject
	if (Input::GetKey('W'))
		ownTransf->Translate(forward * deltaTime * speed);
	if (Input::GetKey('S'))
		ownTransf->Translate(-forward * deltaTime * speed);
	if (Input::GetKey('D'))
		ownTransf->Rotate(vec3(0, -rotSpeed * deltaTime, 0));
	if (Input::GetKey('A'))
		ownTransf->Rotate(vec3(0, rotSpeed * deltaTime, 0));
	
	// terrain walking
	float yOffset = owner->GetCenter().y * ownTransf->GetScale().y;
	Terrain *terrain = Game::GetInstance()->GetTerrain(ownTransf->GetPos());
	vec3 curPos = ownTransf->GetPos();
	curPos.y = terrain->GetHeight(curPos) + yOffset;
	ownTransf->SetPos(curPos);

	// terrain facing
	vec3 newUp = terrain->GetNormal(curPos);
	ownTransf->RotateToUp(newUp);

	// 3rd person camera
	vec2 deltaPos = Input::GetMouseDelta() * Game::GetInstance()->GetSensitivity();
	const float camDist = 3;

	angles += vec3(deltaPos.y, -deltaPos.x, 0) * deltaTime;
	vec3 camPos = curPos - vec3(0, 0, camDist);
	camPos = Transform::RotateAround(camPos, curPos, angles);

	// making sure we don't go under terrain
	float heightAtCamPos = terrain->GetHeight(camPos);
	if (camPos.y < heightAtCamPos)
		camPos.y = heightAtCamPos + 0.1f; 

	camTransf->SetPos(camPos);
	camTransf->LookAt(ownTransf->GetPos());

	if(shootTimer > 0)
		shootTimer -= deltaTime;

	if (shootTimer <= 0)
	{
		if (holdStart == -1 && Input::GetMouseBtn(0))
			holdStart = Game::GetInstance()->GetTime();

		if (holdStart >= 0 && !Input::GetMouseBtn(0))
		{
			float holdLength = Game::GetInstance()->GetTime() - holdStart;
			if (holdLength > 2)
				holdLength = 2;
			float force = mix(5, 20, holdLength / 2);

			vec3 spawnPos = curPos + forward * 0.6f + vec3(0, 0.25f, 0);
			GameObject *go = Game::GetInstance()->Instantiate(spawnPos, vec3(), vec3(0.1f));
			go->AddComponent(new Renderer(3, 0, 6));
			vec3 shootDir = forward + vec3(0, 0.2f, 0);
			go->AddComponent(new Missile(shootDir * force, owner, true));

			Audio::Play(1, curPos);

			shootTimer = shootRate;
			holdStart = -1;
		}
	}
}