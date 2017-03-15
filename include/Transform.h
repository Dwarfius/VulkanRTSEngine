#ifndef _TRANSFORM_T
#define _TRANSFORM_T

#include <glm\glm.hpp>
#include <glm\gtx\transform.hpp>

using namespace glm;

class Transform
{
public:
	Transform();

	vec3 GetForward() { if (dirtyDirs) UpdateRot(); return forward; }
	vec3 GetRight() { if (dirtyDirs) UpdateRot(); return right; }
	vec3 GetUp() { if (dirtyDirs) UpdateRot(); return up; }

	vec3 GetPos() const { return pos; }
	void Translate(vec3 delta) { pos += delta; dirtyModel = true; }
	void SetPos(vec3 newPos) { pos = newPos; dirtyModel = true; }

	vec3 GetScale() const { return size; }
	void SetScale(vec3 pScale) { size = pScale; dirtyModel = true; }
	void AddScale(vec3 delta) { size += delta; dirtyModel = true; }

	void LookAt(vec3 target);

	float GetYaw() const { return yaw; }
	void SetYaw(float newYaw) { yaw = newYaw; dirtyDirs = true; }

	float GetPitch() const { return pitch; }
	void SetPitch(float newPitch) { pitch = newPitch; dirtyDirs = true; }

	float GetRoll() const { return roll; }
	void SetRoll(float newRoll) { roll = newRoll; dirtyDirs = true; }

	void Rotate(float deltaYaw, float deltaPitch, float deltaRoll) { yaw += deltaYaw; pitch += deltaPitch; roll += deltaRoll; dirtyDirs = true; }
	void SetRotation(float y, float p, float r) { yaw = y; pitch = p; roll = r; dirtyDirs = true; }
	void SetRotation(vec3 r) { yaw = r.x; pitch = r.y; roll = r.z; dirtyDirs = true; }

	mat4 GetModelMatrix(vec3 center) { if (dirtyModel || dirtyDirs) UpdateModel(center); return modelM; }

private:
	vec3 pos;
	vec3 size;
	float yaw, pitch, roll;
	vec3 up, forward, right;

	bool dirtyDirs, dirtyModel;
	mat4 rotM, modelM;

	void UpdateRot();
	void UpdateModel(vec3 center);
};

#endif