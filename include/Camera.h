#ifndef _CAMERA_H
#define _CAMERA_H

#include "Common.h"

struct Frustrum
{
	vec3 camPos;
	vec3 X, Y, Z; //camera directions
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
		tang = tan(radians(fov));
		sphereFactorY = 1.0f / cos(fov);

		//computing horizontal fov
		float angleX = atan(tang * ratio);
		sphereFactorX = 1.0f / cos(angleX);
	}

	//updates the frustrums directions and position
	void UpdateFrustrum(const vec3& pos, const vec3& right, const vec3& up, const vec3& forward)
	{
		this->camPos = pos;

		X = right;
		Y = up;
		Z = forward;
	}

	//check if sphere is intersecting the frustrum
	bool CheckSphere(const vec3& pos, const float rad)
	{
		vec3 v = pos - camPos;
		float vZ = dot(v, Z);
		if (vZ < nearPlane - rad || vZ > farPlane + rad)
			return false;

		float vY = dot(v, Y);
		float height = vZ * tang;
		float d = sphereFactorY * rad; //distance from sphere center to yPlane
		if (vY < -height - d || vY > height + d)
			return false;

		float vX = dot(v, X);
		float width = height * ratio;
		d = sphereFactorX * rad; //distance from sphere center to xPlane
		if (vX < -width - d || vX > width + d)
			return false;

		return true;
	}
};

class Camera
{
private:
	const GLfloat sensX = 1, sensY = 1;
	GLfloat yaw, pitch;
	vec3 pos, up, forward, right;
	mat4 projMatrix, viewMatrix, VP;

	bool orthoMode = false;
	Frustrum frustrum;

public:
	Camera(bool orthoMode = false);
	~Camera();

	mat4 Get() { return VP; }

	vec3 GetForward() { return forward; }
	vec3 GetRight() { return right; }
	vec3 GetUp() { return up; }

	vec3 GetPos() { return pos; }
	void Translate(vec3 delta) { pos += delta; }
	void SetPos(vec3 newPos) { pos = newPos; }

	void LookAt(vec3 target) { forward = normalize(target - pos); }

	void Rotate(GLfloat deltaYaw, GLfloat deltaPitch) { yaw += deltaYaw * sensX; pitch += deltaPitch * sensY; }

	void Recalculate();
	mat4 GetView() { return viewMatrix; }
	mat4 GetProj() { return projMatrix; }

	bool IsOrtho() { return orthoMode; }

	//sets the perspective projection
	void SetProjPersp(float fov, float ratio, float nearPlane, float farPlane);
	//sets the ortho projection
	void SetProjOrtho(float left, float right, float bottom, float top);

	bool CheckSphere(const vec3& pos, const float rad) { return frustrum.CheckSphere(pos, rad); }
};
#endif