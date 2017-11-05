#pragma once

class Transform
{
public:
	Transform();
	Transform(vec3 pos, vec3 rot, vec3 scale);

	vec3 GetForward() { if (dirtyDirs) UpdateRot(); return forward; }
	vec3 GetRight() { if (dirtyDirs) UpdateRot(); return right; }
	vec3 GetUp() { if (dirtyDirs) UpdateRot(); return up; }

	vec3 GetPos() const { return myPos; }
	void Translate(vec3 delta) { myPos += delta; dirtyModel = true; }
	void SetPos(vec3 newPos) { myPos = newPos; dirtyModel = true; }

	vec3 GetScale() const { return myScale; }
	void SetScale(vec3 pScale) { myScale = pScale; dirtyModel = true; }
	void AddScale(vec3 delta) { myScale += delta; dirtyModel = true; }

	void LookAt(vec3 target);
	void RotateToUp(vec3 newUp);

	quat GetRotation() { return myRotation; }
	vec3 GetEuler() { return degrees(eulerAngles(myRotation)); }
	void Rotate(vec3 deltaEuler) { SetRotation(quat(radians(deltaEuler)) * myRotation); }
	void SetRotation(vec3 euler) { SetRotation(quat(radians(euler))); }
	void SetRotation(quat rot) { myRotation = rot; dirtyDirs = true; }

	mat4 GetModelMatrix(vec3 center) { if (dirtyModel || dirtyDirs) UpdateModel(center); return modelM; }

	static vec3 RotateAround(vec3 point, vec3 refPoint, vec3 angles);

private:
	vec3 myPos;
	vec3 myScale;
	quat myRotation;
	vec3 up, forward, right;

	bool dirtyDirs, dirtyModel;
	mat4 rotM, modelM;

	void UpdateRot();
	void UpdateModel(vec3 center);

	quat RotationBetweenVectors(vec3 start, vec3 dest);
};