#pragma once

#include "Transform.h"

struct Frustrum
{
	glm::vec3 camPos;
	glm::vec3 X, Y, Z; //camera directions
	float nearPlane, farPlane, width, height, ratio;

	//used to determine the width and height at distance between planes
	float sphereFactorX, sphereFactorY;

	//tangent used to calculate the height
	float tang;

	//sets the frustrums parameters - near/far planes and near width/height
	void SetFrustrumDef(float fov, float ratio, float nearPlane, float farPlane)
	{
		this->ratio = ratio;
		this->nearPlane = nearPlane;
		this->farPlane = farPlane;

		//computing the vertical fov
		tang = tan(glm::radians(fov));
		sphereFactorY = 1.0f / cos(glm::radians(fov));

		//computing horizontal fov
		float angleX = atan(tang * ratio);
		sphereFactorX = 1.0f / cos(angleX);
	}

	//updates the frustrums directions and position
	void UpdateFrustrum(const glm::vec3& pos, const glm::vec3& right, const glm::vec3& up, const glm::vec3& forward)
	{
		this->camPos = pos;

		X = right;
		Y = up;
		Z = forward;
	}

	//check if sphere is intersecting the frustrum
	bool CheckSphere(const glm::vec3& pos, const float rad) const
	{
		const glm::vec3 v = pos - camPos;
		const float vZ = glm::dot(v, Z);
		if (vZ < nearPlane - rad || vZ > farPlane + rad)
			return false;

		const float vY = glm::dot(v, Y);
		const float height = vZ * tang;
		float d = sphereFactorY * rad; //distance from sphere center to yPlane
		if (vY < -height - d || vY > height + d)
			return false;

		const float vX = glm::dot(v, X);
		const float width = height * ratio;
		d = sphereFactorX * rad; //distance from sphere center to xPlane
		if (vX < -width - d || vX > width + d)
			return false;

		return true;
	}
};

class Camera
{
public:
	Camera(float aWidth, float aHeight, bool anOrthoMode = false);
	~Camera() {}

	glm::mat4 Get() const { return myVP; }

	const Transform& GetTransform() const { return myTransform; }
	Transform& GetTransform() { return myTransform; }

	void Recalculate();
	glm::mat4 GetView() const { return myViewMatrix; }
	glm::mat4 GetProj() const { return myProjMatrix; }
	void InvertProj() { myProjMatrix[1][1] *= -1; }

	bool IsOrtho() const { return myOrthoMode; }

	//sets the perspective projection
	void SetProjPersp(float aFov, float aRatio, float aNearPlane, float fFarPlane);
	//sets the ortho projection
	void SetProjOrtho(float aLeft, float aRight, float aBottom, float aTop);

	// checks a bounding sphere against the camera's frustum
	bool CheckSphere(const glm::vec3& aPos, const float aRad) const { return myFrustrum.CheckSphere(aPos, aRad); }

private:
	const float sensX = 1, sensY = 1;
	Transform myTransform;
	glm::mat4 myProjMatrix, myViewMatrix, myVP;

	bool myOrthoMode = false;
	Frustrum myFrustrum;
};