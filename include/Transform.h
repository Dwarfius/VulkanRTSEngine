#ifndef _TRANSFORM_T
#define _TRANSFORM_T

#include <glm\glm.hpp>
#include <glm\gtx\transform.hpp>
#include <glm\gtx\quaternion.hpp>

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

	vec3 GetRotation() { return euler; }
	void Rotate(vec3 deltaEuler) { euler += deltaEuler; dirtyDirs = true; }
	void SetRotation(vec3 euler) { this->euler = euler; dirtyDirs = true; }

	mat4 GetModelMatrix(vec3 center) { if (dirtyModel || dirtyDirs) UpdateModel(center); return modelM; }

private:
	vec3 pos;
	vec3 size;

	vec3 euler;
	quat quaternion;
	vec3 up, forward, right;

	bool dirtyDirs, dirtyModel;
	mat4 rotM, modelM;

	void UpdateRot();
	void UpdateModel(vec3 center);
};

#endif