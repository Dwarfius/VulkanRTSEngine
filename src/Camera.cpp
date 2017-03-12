#include "Camera.h"
#include "Common.h"
#include "Game.h"
#include "Graphics.h"

Camera::Camera(bool orthoMode)
{
	yaw = -90; 
	pitch = 0;
	pos = vec3(0, 0, 0);
	up = vec3(0, 1, 0);

	//setting up the matrix for UI rendering
	//call Recalculate to get proper perspective matrix
	this->orthoMode = orthoMode;
	if (orthoMode)
		SetProjOrtho(0, Game::GetGraphics()->GetWidth(), 0, Game::GetGraphics()->GetHeight());
	else
		SetProjPersp(45, Game::GetGraphics()->GetWidth() / Game::GetGraphics()->GetHeight(), 0.1f, 1000.f);
}

void Camera::LookAt(vec3 target)
{ 
	// fix this!
	forward = normalize(target - pos);
	pitch = degrees(asin(-forward.y));
	yaw = degrees(atan2(forward.z, forward.x));
}

void Camera::Recalculate()
{
	if (pitch > 89.f) //setting top/bottom boundaries
		pitch = 89.f;
	else if (pitch < -89.f)
		pitch = -89.f;

	float pitchRad = radians(pitch);
	float yawRad = radians(yaw);

	forward.x = cos(yawRad) * cos(pitchRad);
	forward.y = sin(pitchRad);
	forward.z = sin(yawRad) * cos(pitchRad);
	forward = normalize(forward);

	right = normalize(cross(vec3(0, 1, 0), -forward));
	up = cross(-forward, right);

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