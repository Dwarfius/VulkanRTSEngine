#include "Common.h"
#include "EditorMode.h"

#include "../Game.h"
#include "../Camera.h"
#include "../Input.h"

void EditorMode::Update(float aDeltaTime)
{
	// FPS camera implementation
	Camera* cam = Game::GetInstance()->GetCamera();
	Transform& camTransf = cam->GetTransform();

	glm::vec3 pos = camTransf.GetPos();
	const glm::vec3 forward = camTransf.GetForward();
	const glm::vec3 right = camTransf.GetRight();
	const glm::vec3 up = camTransf.GetUp();
	if (Input::GetKey('W'))
	{
		pos += forward * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('S'))
	{
		pos -= forward * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('D'))
	{
		pos += right * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('A'))
	{
		pos -= right * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('R'))
	{
		pos += up * myFlightSpeed * aDeltaTime;
	}
	if (Input::GetKey('F'))
	{
		pos -= up * myFlightSpeed * aDeltaTime;
	}
	camTransf.SetPos(pos);

	// to avoid accumulating roll from quaternion applications, have to apply then separately
	const glm::vec2 mouseDelta = Input::GetMouseDelta();
	
	glm::vec3 pitchDelta(mouseDelta.y, 0.f, 0.f);
	pitchDelta *= myMouseSensitivity;
	const glm::quat pitchRot(glm::radians(pitchDelta));

	glm::vec3 yawDelta(0.f, -mouseDelta.x, 0.f);
	yawDelta *= myMouseSensitivity;
	const glm::quat yawRot(glm::radians(yawDelta));

	camTransf.SetRotation(yawRot * camTransf.GetRotation() * pitchRot);
}