#include "Common.h"
#include "Camera.h"
#include "Game.h"
#include "Graphics.h"

Camera::Camera(float width, float height, bool orthoMode)
	: myOrthoMode(orthoMode)
	, myTransform()
{
	//setting up the matrix for UI rendering
	//call Recalculate to get proper perspective matrix
	if (orthoMode)
	{
		SetProjOrtho(0, width, 0, height);
	}
	else
	{
		SetProjPersp(45, width / height, 0.1f, 1000.f);
	}
}

void Camera::Recalculate()
{
	glm::vec3 pos = myTransform.GetPos();
	glm::vec3 right = myTransform.GetRight();
	glm::vec3 up = myTransform.GetUp();
	glm::vec3 forward = myTransform.GetForward();
	myFrustrum.UpdateFrustrum(pos, right, up, forward);
	myViewMatrix = glm::lookAt(pos, pos + forward, up);

	myVP = myProjMatrix * myViewMatrix;
}

void Camera::SetProjPersp(float fov, float ratio, float nearPlane, float farPlane)
{
	myOrthoMode = false;
	
	myProjMatrix = glm::perspective(fov, ratio, nearPlane, farPlane);
	myFrustrum.SetFrustrumDef(fov, ratio, nearPlane, farPlane);
}

void Camera::SetProjOrtho(float left, float right, float bottom, float top)
{
	myOrthoMode = true;

	myProjMatrix = glm::ortho(left, right, bottom, top);
	myVP = myProjMatrix;
}
