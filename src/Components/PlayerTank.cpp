#include "Components\PlayerTank.h"
#include "Game.h"
#include "Input.h"

void PlayerTank::Update(float deltaTime)
{
	// just general settings
	const float speed = 1;
	const float mouseSens = 0.2f;

	Camera *cam = Game::GetInstance()->GetCamera();
	Transform *camTransf = cam->GetTransform();
	Transform *ownTransf = owner->GetTransform();

	// update the camera
	vec3 forward = camTransf->GetForward();
	vec3 right = camTransf->GetRight();
	vec3 up = camTransf->GetUp();

	// move the gameobject
	if (Input::GetKey('W'))
		camTransf->Translate(forward * deltaTime * speed);
	if (Input::GetKey('S'))
		camTransf->Translate(-forward * deltaTime * speed);
	if (Input::GetKey('D'))
		camTransf->Translate(right * deltaTime * speed);
	if (Input::GetKey('A'))
		camTransf->Translate(-right * deltaTime * speed);
	if (Input::GetKey('Q'))
		camTransf->Translate(-up * deltaTime * speed);
	if (Input::GetKey('E'))
		camTransf->Translate(up * deltaTime * speed);
	if (Input::GetKey('L'))
		camTransf->LookAt(ownTransf->GetPos());

	vec2 deltaPos = Input::GetMouseDelta();
	camTransf->Rotate(-deltaPos.x * mouseSens, deltaPos.y * mouseSens, 0);

	Terrain *terrain = Game::GetInstance()->GetTerrain(owner->GetTransform()->GetPos());
	vec3 curPos = camTransf->GetPos();
	curPos.y = terrain->GetHeight(curPos) + 1;
	camTransf->SetPos(curPos);
}