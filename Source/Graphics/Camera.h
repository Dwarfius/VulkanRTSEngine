#pragma once

#include <Core/Transform.h>

struct Frustum
{
	glm::vec4 myPlanes[6]; // left, right, bottom, top, far, near

	// taken from NVidia's tesselated terrain whitepaper implementation
	// calculate frustum planes (in world space) from current 
	// projection and view matrices
	// http://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
	void UpdateFrustumPlanes(const glm::mat4& aViewProjMat)
	{
		const glm::vec4 rowX = GetRow(aViewProjMat, 0);
		const glm::vec4 rowY = GetRow(aViewProjMat, 1);
		const glm::vec4 rowZ = GetRow(aViewProjMat, 2);
		const glm::vec4 rowW = GetRow(aViewProjMat, 3);
		myPlanes[0] = rowW + rowX;   // left
		myPlanes[1] = rowW - rowX;   // right
		myPlanes[2] = rowW + rowY;   // bottom
		myPlanes[3] = rowW - rowY;   // top
		myPlanes[4] = rowW + rowZ;   // far
		myPlanes[5] = rowW - rowZ;   // near
		// normalize planes
		for (int i = 0; i < 6; i++) 
		{
			const float l = glm::length(glm::vec3(myPlanes[i]));
			myPlanes[i] = myPlanes[i] / l;
		}
	}

	// taken from NVidia's tesselated terrain whitepaper implementation
	// check if sphere is intersecting the frustrum
	bool CheckSphere(glm::vec3 aPos, float aRadius) const
	{
		const glm::vec4 pos = glm::vec4(aPos, 1.0);
		for (int i = 0; i < 6; i++) 
		{
			if (glm::dot(pos, myPlanes[i]) + aRadius < 0.0)
			{
				// sphere outside plane
				return false;
			}
		}
		return true;
	}

private:
	static glm::vec4 GetRow(const glm::mat4& aMat, int aRow)
	{
		return glm::vec4(	
			aMat[0][aRow],
			aMat[1][aRow],
			aMat[2][aRow],
			aMat[3][aRow]
		);
	}
};

class Camera
{
public:
	constexpr static float kNearPlane = 0.1f;
	constexpr static float kFarPlane = 10000.f;
	constexpr static float kFOV = 45;

	Camera(float aWidth, float aHeight, bool anOrthoMode = false);
	~Camera() {}

	glm::mat4 Get() const { return myVP; }

	const Transform& GetTransform() const { return myTransform; }
	Transform& GetTransform() { return myTransform; }

	void Recalculate(float aWidth, float aHeight);
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