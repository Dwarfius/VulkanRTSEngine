#include "Components\PlayerTank.h"
#include "Game.h"
#include "Input.h"
#include "Components\Tank.h"

void PlayerTank::Update(float deltaTime)
{
	// just general settings
	const float speed = 0.5f;
	const float mouseSens = 1.f; //0.75f;

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
	vec3 camPos = ownTransf->GetPos() - dist * camOffset;

	// making sure we don't go under terrain
	float heightAtCamPos = terrain->GetHeight(camPos);
	if (camPos.y < heightAtCamPos)
		camPos.y = heightAtCamPos + 0.1f; 

	camTransf->SetPos(camPos);
	camTransf->LookAt(ownTransf->GetPos());

	if(Input::GetMouseBtn(0))
	{
		vec3 pos = vec3(rand() % 100 - 50, 1, rand() % 100 - 50);
		GameObject *go = Game::GetInstance()->Instantiate(pos, vec3(), vec3(0.005f));
		go->AddComponent(new Renderer(2, 0, 5));
		go->AddComponent(new Tank());
	}
}

void PlayerTank::OnCollideWithTerrain()
{
	//printf("[Info] Colliding with terrain\n");
}

void PlayerTank::OnCollideWithGO(GameObject *other)
{
	//printf("[Info] Colliding with other GO\n");
}