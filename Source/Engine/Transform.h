#pragma once

class Transform
{
public:
	Transform();
	Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);

	glm::vec3 GetForward() { if (dirtyDirs) UpdateRot(); return forward; }
	glm::vec3 GetRight() { if (dirtyDirs) UpdateRot(); return right; }
	glm::vec3 GetUp() { if (dirtyDirs) UpdateRot(); return up; }

	glm::vec3 GetPos() const { return myPos; }
	void Translate(glm::vec3 delta) { myPos += delta; dirtyModel = true; }
	void SetPos(glm::vec3 newPos) { myPos = newPos; dirtyModel = true; }

	glm::vec3 GetScale() const { return myScale; }
	void SetScale(glm::vec3 pScale) { myScale = pScale; dirtyModel = true; }
	void AddScale(glm::vec3 delta) { myScale += delta; dirtyModel = true; }

	void LookAt(glm::vec3 target);
	void RotateToUp(glm::vec3 newUp);

	glm::quat GetRotation() { return myRotation; }
	glm::vec3 GetEuler() { return glm::degrees(glm::eulerAngles(myRotation)); }
	void Rotate(glm::vec3 deltaEuler) { SetRotation(glm::quat(glm::radians(deltaEuler)) * myRotation); }
	void SetRotation(glm::vec3 euler) { SetRotation(glm::quat(glm::radians(euler))); }
	void SetRotation(glm::quat rot) { myRotation = rot; dirtyDirs = true; }

	const glm::mat4& GetModelMatrix(glm::vec3 center);

	// Returns a new point, which is calculated by rotating point around a refPoint using angles
	static glm::vec3 RotateAround(glm::vec3 point, glm::vec3 refPoint, glm::vec3 angles);

private:
	glm::vec3 myPos;
	glm::vec3 myScale;
	glm::quat myRotation;
	glm::vec3 up, forward, right;

	bool dirtyDirs, dirtyModel;
	glm::mat4 rotM, modelM;

	void UpdateRot();
	void UpdateModel(glm::vec3 center);

	glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);
};