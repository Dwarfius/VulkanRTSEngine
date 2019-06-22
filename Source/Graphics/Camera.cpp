#include "Precomp.h"
#include "Camera.h"

Camera::Camera(float aWidth, float aHeight, bool anOrthoMode /* =false */)
	: myOrthoMode(anOrthoMode)
	, myTransform()
{
	//setting up the matrix for UI rendering
	//call Recalculate to get proper perspective matrix
	if (myOrthoMode)
	{
		SetProjOrtho(0, aWidth, 0, aHeight);
	}
	else
	{
		SetProjPersp(45, aWidth / aHeight, 0.1f, 1000.f);
	}
}

void Camera::Recalculate()
{
	glm::vec3 pos = myTransform.GetPos();
	glm::vec3 right = myTransform.GetRight();
	glm::vec3 up = myTransform.GetUp();
	glm::vec3 forward = myTransform.GetForward();
	myViewMatrix = glm::lookAt(pos, pos + forward, up);
	myVP = myProjMatrix * myViewMatrix;
	myFrustum.UpdateFrustumPlanes(myVP);
}

void Camera::SetProjPersp(float aFov, float aRatio, float aNearPlane, float fFarPlane)
{
	myOrthoMode = false;
	
	myProjMatrix = glm::perspective(aFov, aRatio, aNearPlane, fFarPlane);
	myVP = myProjMatrix * myViewMatrix;
	myFrustum.UpdateFrustumPlanes(myProjMatrix);
}

void Camera::SetProjOrtho(float aLeft, float aRight, float aBottom, float aTop)
{
	myOrthoMode = true;

	myProjMatrix = glm::ortho(aLeft, aRight, aBottom, aTop);
	myVP = myProjMatrix;
}
