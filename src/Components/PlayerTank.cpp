#include "Components\PlayerTank.h"
#include "Game.h"
#include "Input.h"

void PlayerTank::Update(float deltaTime)
{
	// just general settings
	const float speed = 1;
	const float mouseSens = 0.75f;

	Camera *cam = Game::GetInstance()->GetCamera();
	Transform *camTransf = cam->GetTransform();
	Transform *ownTransf = owner->GetTransform();

	// update the camera
	vec3 forward = camTransf->GetForward();
	vec3 right = camTransf->GetRight();
	vec3 up = camTransf->GetUp();

	// move the gameobject
	if (Input::GetKey('W'))
		ownTransf->Translate(forward * deltaTime * speed);
	if (Input::GetKey('S'))
		ownTransf->Translate(-forward * deltaTime * speed);
	if (Input::GetKey('D'))
		ownTransf->Translate(right * deltaTime * speed);
	if (Input::GetKey('A'))
		ownTransf->Translate(-right * deltaTime * speed);
	if (Input::GetKey('Q'))
		ownTransf->Translate(-up * deltaTime * speed);
	if (Input::GetKey('E'))
		ownTransf->Translate(up * deltaTime * speed);
	
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
	vec2 deltaPos = Input::GetMouseDelta();
	const float dist = 2;

	ownTransf->Rotate(vec3(0, -deltaPos.x * mouseSens * 10.f * deltaTime, 0));
	vec3 tankForward = ownTransf->GetForward();
	heightOffset -= deltaPos.y * mouseSens * 0.1f * deltaTime;
	heightOffset = clamp(heightOffset, 0.f, 1.f);

	vec3 camOffset = normalize(vec3(tankForward.x, -0.5f + heightOffset, tankForward.z));
	camTransf->SetPos(ownTransf->GetPos() - dist * camOffset);
	camTransf->LookAt(ownTransf->GetPos());

	/*curPos = camTransf->GetPos();
	curPos.y = terrain->GetHeight(curPos) + 1;
	camTransf->SetPos(curPos);
	camTransf->Rotate(vec3(deltaPos.y * mouseSens, -deltaPos.x * mouseSens, 0) * deltaTime);
	vec3 r = camTransf->GetEuler();
	printf("[Info] rot: %f %f %f\n", r.x, r.y, r.z);*/
}