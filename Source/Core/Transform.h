#pragma once

// class that exposes utility functions around translation, rotation and scaling
// built around the right-hand coordinate system
class Transform
{
public:
	// Creates zero-initialized transform (identity matrix)
	Transform();
	// Creates a transform from position, rotation (in radians) and scale
	Transform(glm::vec3 aPos, glm::vec3 aRot, glm::vec3 aScale);

	// optional, allows to set a pivot for a transform
	void SetCenter(glm::vec3 aCenter) { myCenter = aCenter; myDirtyModel = true; }

	glm::vec3 GetForward() { if (myDirtyDirs) UpdateRot(); return myForward; }
	glm::vec3 GetRight() { if (myDirtyDirs) UpdateRot(); return myRight; }
	glm::vec3 GetUp() { if (myDirtyDirs) UpdateRot(); return myUp; }

	glm::vec3 GetPos() const { return myPos; }
	void Translate(glm::vec3 aDelta) { myPos += aDelta; myDirtyModel = true; }
	void SetPos(glm::vec3 aNewPos) { myPos = aNewPos; myDirtyModel = true; }

	glm::vec3 GetScale() const { return myScale; }
	void SetScale(glm::vec3 aScale) { myScale = aScale; myDirtyModel = true; }

	void LookAt(glm::vec3 aTarget);
	void RotateToUp(glm::vec3 aNewUp);

	glm::quat GetRotation() const { return myRotation; }
	glm::vec3 GetEuler() const { return glm::degrees(glm::eulerAngles(myRotation)); }
	void Rotate(glm::vec3 aDeltaEuler) { SetRotation(glm::quat(glm::radians(aDeltaEuler)) * myRotation); }
	void SetRotation(glm::vec3 anEuler) { SetRotation(glm::quat(glm::radians(anEuler))); }
	void SetRotation(glm::quat aRot) { myRotation = aRot; myDirtyDirs = true; }

	// TODO: need to get rid of lazy transforms
	// not every object in the game is going to be moving, but every one will call update
	// so that generates unnecessary overhead
	void UpdateModel();
	const glm::mat4& GetModelMatrix() const { return myModelM; }

	// Returns a new point, which is calculated by rotating point around a refPoint using angles
	static glm::vec3 RotateAround(glm::vec3 aPoint, glm::vec3 aRefPoint, glm::vec3 anAngles);

private:
	// TODO: need to get rid of most of this
	glm::mat4 myRotM, myModelM;

	glm::vec3 myPos;
	glm::vec3 myScale;
	glm::quat myRotation;
	glm::vec3 myUp, myForward, myRight;
	glm::vec3 myCenter;

	bool myDirtyDirs, myDirtyModel;

	void UpdateRot();

	glm::quat RotationBetweenVectors(glm::vec3 aStart, glm::vec3 aDest);
};