#ifndef _CAMERA_H
#define _CAMERA_H

#include <glm\glm.hpp>

using namespace glm;

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
		sphereFactorY = 1.0f / cos(radians(fov));

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
	bool CheckSphere(const vec3& pos, const float rad) const
	{
		const vec3 v = pos - camPos;
		const float vZ = dot(v, Z);
		if (vZ < nearPlane - rad || vZ > farPlane + rad)
			return false;

		const float vY = dot(v, Y);
		const float height = vZ * tang;
		float d = sphereFactorY * rad; //distance from sphere center to yPlane
		if (vY < -height - d || vY > height + d)
			return false;

		const float vX = dot(v, X);
		const float width = height * ratio;
		d = sphereFactorX * rad; //distance from sphere center to xPlane
		if (vX < -width - d || vX > width + d)
			return false;

		return true;
	}
};

class Camera
{
private:
	const float sensX = 1, sensY = 1;
	float yaw, pitch;
	vec3 pos, up, forward, right;
	mat4 projMatrix, viewMatrix, VP;

	bool orthoMode = false;
	Frustrum frustrum;

public:
	Camera(bool orthoMode = false);
	~Camera() {}

	mat4 Get() const { return VP; }

	vec3 GetForward() const { return forward; }
	vec3 GetRight() const { return right; }
	vec3 GetUp() const { return up; }

	vec3 GetPos() const { return pos; }
	void Translate(vec3 delta) { pos += delta; }
	void SetPos(vec3 newPos) { pos = newPos; }

	void LookAt(vec3 target);

	void Rotate(float deltaYaw, float deltaPitch) { yaw += deltaYaw * sensX; pitch += deltaPitch * sensY; }

	void Recalculate();
	mat4 GetView() const { return viewMatrix; }
	mat4 GetProj() const { return projMatrix; }

	bool IsOrtho() const { return orthoMode; }

	//sets the perspective projection
	void SetProjPersp(float fov, float ratio, float nearPlane, float farPlane);
	//sets the ortho projection
	void SetProjOrtho(float left, float right, float bottom, float top);

	bool CheckSphere(const vec3& pos, const float rad) const { return frustrum.CheckSphere(pos, rad); }
};
#endif