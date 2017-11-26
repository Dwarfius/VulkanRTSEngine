#include "Common.h"
#include "Camera.h"
#include "Game.h"
#include "Graphics.h"

Camera::Camera(float width, float height, bool orthoMode)
{
	//setting up the matrix for UI rendering
	//call Recalculate to get proper perspective matrix
	this->orthoMode = orthoMode;
	if (orthoMode)
		SetProjOrtho(0, width, 0, height);
	else
		SetProjPersp(45, width / height, 0.1f, 1000.f);
}

void Camera::Recalculate()
{
	vec3 angles = transf.GetEuler();
	transf.SetRotation(angles);

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
