#pragma once

#include <Core/Transform.h>

struct Frustum
{
	glm::vec3 myCamPos;
	glm::vec3 myRight, myUp, myForward; //camera directions
	float myNearPlane, myFarPlane, myRatio;

	//used to determine the width and height at distance between planes
	float mySphereFactorX, mySphereFactorY;

	//tangent used to calculate the height
	float myTangent;

	//sets the frustrums parameters - near/far planes and near width/height
	void SetFrustrumDef(float aFov, float aRatio, float aNearPlane, float aFarPlane)
	{
		myRatio = aRatio;
		myNearPlane = aNearPlane;
		myFarPlane = aFarPlane;

		//computing the vertical fov
		myTangent = glm::tan(glm::radians(aFov));
		mySphereFactorY = 1.0f / glm::cos(glm::radians(aFov));

		//computing horizontal fov
		const float angleX = glm::atan(myTangent * myRatio);
		mySphereFactorX = 1.0f / glm::cos(angleX);
	}

	//updates the frustrums directions and position
	void UpdateFrustrum(glm::vec3 aPos, glm::vec3 aRight, glm::vec3 anUp, glm::vec3 aForward)
	{
		myCamPos = aPos;

		myRight = aRight;
		myUp = anUp;
		myForward = aForward;
	}

	//check if sphere is intersecting the frustrum
	bool CheckSphere(glm::vec3 aPos, float aRadius) const
	{
		const glm::vec3 v = aPos - myCamPos;
		const float vZ = glm::dot(v, myForward);
		if (vZ < myNearPlane - aRadius || vZ > myFarPlane + aRadius)
		{
			return false;
		}

		const float vY = glm::dot(v, myUp);
		const float height = vZ * myTangent;
		float d = mySphereFactorY * aRadius; //distance from sphere center to yPlane
		if (vY < -height - d || vY > height + d)
		{
			return false;
		}

		const float vX = glm::dot(v, myRight);
		const float width = height * myRatio;
		d = mySphereFactorX * aRadius; //distance from sphere center to xPlane
		if (vX < -width - d || vX > width + d)
		{
			return false;
		}

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
	bool CheckSphere(const glm::vec3& aPos, const float aRad) const { return myFrustum.CheckSphere(aPos, aRad); }
	const Frustum& GetFrustum() const { return myFrustum; }

private:
	Transform myTransform;
	glm::mat4 myProjMatrix, myViewMatrix, myVP;

	bool myOrthoMode = false;
	Frustum myFrustum;
};