#include "Components\PlayerTank.h"
#include "Game.h"
#include "Input.h"

void PlayerTank::Update(float deltaTime)
{
	// just general settings
	const float speed = 1;
	const float mouseSens = 0.2f;

	Camera *cam = Game::GetInstance()->GetCamera();

	// update the camera
	vec3 forward = cam->GetForward();
	vec3 right = cam->GetRight();
	vec3 up = cam->GetUp();
	if (Input::GetKey('W'))
		cam->Translate(forward * deltaTime * speed);
	if (Input::GetKey('S'))
		cam->Translate(-forward * deltaTime * speed);
	if (Input::GetKey('D'))
		cam->Translate(right * deltaTime * speed);
	if (Input::GetKey('A'))
		cam->Translate(-right * deltaTime * speed);
	if (Input::GetKey('Q'))
		cam->Translate(-up * deltaTime * speed);
	if (Input::GetKey('E'))
		cam->Translate(up * deltaTime * speed);

	vec2 deltaPos = Input::GetMouseDelta();
	cam->Rotate(deltaPos.x * mouseSens, -deltaPos.y * mouseSens);

	Terrain *terrain = Game::GetInstance()->GetTerrain(owner->GetPos());
	vec3 curPos = cam->GetPos();
	curPos.y = terrain->GetHeight(curPos) + 1;
	cam->SetPos(curPos);
}