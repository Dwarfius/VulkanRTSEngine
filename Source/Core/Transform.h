#pragma once

class Serializer;

// class that exposes utility functions around translation, rotation and scaling
// built around the left-hand coordinate system. Provides pivot functionality
class Transform
{
public:
	// Creates zero-initialized transform with scale of 1
	Transform();
	Transform(glm::vec3 aPos, glm::quat aRot, glm::vec3 aScale);

	glm::vec3 GetRight() const { return myRotation * glm::vec3(1, 0, 0); }
	glm::vec3 GetForward() const { return myRotation * glm::vec3(0, 1, 0); }
	glm::vec3 GetUp() const { return myRotation * glm::vec3(0, 0, 1); }

	glm::vec3 GetPos() const { return myPos; }
	void Translate(glm::vec3 aDelta) { myPos += aDelta; }
	void SetPos(glm::vec3 aNewPos) { myPos = aNewPos; }

	glm::vec3 GetScale() const { return myScale; }
	void SetScale(glm::vec3 aScale) { myScale = aScale; }

	void LookAt(glm::vec3 aTarget);
	void RotateToUp(glm::vec3 aNewUp);

	glm::quat GetRotation() const { return myRotation; }
	// Returns euler angles in radians
	glm::vec3 GetEuler() const { return glm::eulerAngles(myRotation); }
	// Given euler angles in radians, rotates the current transform
	void Rotate(glm::vec3 aDeltaEuler) { SetRotation(glm::quat(aDeltaEuler) * myRotation); }
	// Sets a new rotation from euler angles in radians
	void SetRotation(glm::vec3 anEuler) { SetRotation(glm::quat(anEuler)); }
	void SetRotation(glm::quat aRot) { myRotation = aRot; }

	// Converts to a global matrix
	glm::mat4 GetMatrix() const;

	Transform operator*(const Transform& aOther) const;

	// Generates an Inverted Transform
	Transform GetInverted() const;

	// Returns a new point, which is calculated by rotating point around a refPoint using angles
	static glm::vec3 RotateAround(glm::vec3 aPoint, glm::vec3 aRefPoint, glm::vec3 anAngles);

	bool operator==(const Transform& aOther) const
	{
		return myPos == aOther.myPos
			&& myScale == aOther.myScale
			&& myRotation == aOther.myRotation;
	}
	bool operator!=(const Transform& aOther) const
	{
		return !(*this == aOther);
	}

	void Serialize(Serializer& aSerializer);

private:
	glm::vec3 myPos;
	glm::vec3 myScale;
	glm::quat myRotation;

	static glm::quat RotationBetweenVectors(glm::vec3 aStart, glm::vec3 aDest);
};