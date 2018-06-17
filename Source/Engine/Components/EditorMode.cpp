#include "Common.h"
#include "EditorMode.h"

#include "../Game.h"
#include "../Camera.h"
#include "../Input.h"

void EditorMode::Update(float deltaTime)
{
	// 3rd person camera
	Camera* cam = Game::GetInstance()->GetCamera();
	Transform* camTransf = cam->GetTransform();

	glm::vec3 pos = camTransf->GetPos();
	glm::vec3 forward = camTransf->GetForward();
	glm::vec3 right = camTransf->GetRight();
	if (Input::GetKey('W'))
	{
		pos += forward * myFlightSpeed * deltaTime;
	}
	if (Input::GetKey('S'))
	{
		pos -= forward * myFlightSpeed * deltaTime;
	}
	if (Input::GetKey('D'))
	{
		pos += right * myFlightSpeed * deltaTime;
	}
	if (Input::GetKey('A'))
	{
		pos -= right * myFlightSpeed * deltaTime;
	}

	// TODO: need to fix this
	glm::vec3 rot = camTransf->GetEuler();
	glm::vec3 delta(Input::GetMouseDelta(), 0);
	rot += glm::vec3(delta.y, delta.x, 0.f) * myMouseSensitivity * deltaTime;

	camTransf->SetPos(pos);
	camTransf->SetRotation(rot);
}