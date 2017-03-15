#include "Camera.h"
#include "Common.h"
#include "Game.h"
#include "Graphics.h"

Camera::Camera(bool orthoMode)
{
	//setting up the matrix for UI rendering
	//call Recalculate to get proper perspective matrix
	this->orthoMode = orthoMode;
	if (orthoMode)
		SetProjOrtho(0, Game::GetGraphics()->GetWidth(), 0, Game::GetGraphics()->GetHeight());
	else
		SetProjPersp(45, Game::GetGraphics()->GetWidth() / Game::GetGraphics()->GetHeight(), 0.1f, 1000.f);
}

void Camera::Recalculate()
{
	if (transf.GetPitch() > 89.f) //setting top/bottom boundaries
		transf.SetPitch(89.f);
	else if (transf.GetPitch() < -89.f)
		transf.SetPitch(-89.f);

	vec3 pos = transf.GetPos();
	vec3 right = transf.GetRight();
	vec3 up = transf.GetUp();
	vec3 forward = transf.GetForward();
	frustrum.UpdateFrustrum(pos, right, up, forward);
	viewMatrix = lookAt(pos, pos + forward, up);

	VP = projMatrix * viewMatrix;
}

void Camera::SetProjPersp(float fov, float ratio, float nearPlane, float farPlane)
{
	orthoMode = false;
	
	projMatrix = perspective(fov, ratio, nearPlane, farPlane);
	frustrum.SetFrustrumDef(fov, ratio, nearPlane, farPlane);
}

void Camera::SetProjOrtho(float left, float right, float bottom, float top)
{
	orthoMode = true;

	projMatrix = ortho(left, right, bottom, top);
	VP = projMatrix;
}