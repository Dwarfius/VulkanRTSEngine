#include "Common.h"
#include "Transform.h"

Transform::Transform()
	: myDirtyModel(true)
	, myDirtyDirs(true)
	, myPos(0.f)
	, myScale(0.f)
	, myRotation(glm::vec3(0.f))
	, myCenter(0.f)
{
}

Transform::Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
	: myDirtyModel(true)
	, myDirtyDirs(true)
	, myPos(pos)
	, myScale(scale)
	, myRotation(glm::vec3(rot))
	, myCenter(0.f)
{
}

void Transform::LookAt(glm::vec3 aTarget)
{
	myForward = glm::normalize(aTarget - myPos);
	if (glm::length2(myForward) < 0.0001f)
	{
		return;
	}

	// Recompute desiredUp so that it's perpendicular to the direction
	// You can skip that part if you really want to force desiredUp
	glm::vec3 desiredUp(0, 1, 0);
	glm::vec3 right = glm::cross(myForward, desiredUp);
	desiredUp = glm::cross(right, myForward);

	// Find the rotation between the front of the object (that we assume towards +Z,
	// but this depends on your model) and the desired direction
	glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 0.0f, 1.0f), myForward);
	// Because of the 1rst rotation, the up is probably completely screwed up. 
	// Find the rotation between the "up" of the rotated object, and the desired up
	glm::vec3 newUp = rot1 * glm::vec3(0.0f, 1.0f, 0.0f);
	glm::quat rot2 = RotationBetweenVectors(newUp, desiredUp);

	// Apply them
	myRotation = rot2 * rot1; // remember, in reverse order.

	UpdateRot();
}

void Transform::RotateToUp(glm::vec3 aNewUp)
{
	glm::quat extraRot = RotationBetweenVectors(GetUp(), aNewUp);
	myRotation = extraRot * myRotation;
	UpdateRot();
}

const glm::mat4& Transform::GetModelMatrix()
{
	if (myDirtyModel || myDirtyDirs)
	{ 
		UpdateModel(); 
	} 
	return myModelM;
}

glm::vec3 Transform::RotateAround(glm::vec3 aPoint, glm::vec3 aRefPoint, glm::vec3 anAngles)
{
	aPoint -= aRefPoint;
	aPoint = glm::quat(anAngles) * aPoint;
	return aPoint + aRefPoint;
}

void Transform::UpdateRot()
{
	// making sure that it's a proper unit quat
	myRotation = glm::normalize(myRotation);
	myRotM = glm::mat4_cast(myRotation);

	myRight		= myRotM * glm::vec4(-1, 0, 0, 0);
	myUp		= myRotM * glm::vec4( 0, 1, 0, 0);
	myForward	= myRotM * glm::vec4( 0, 0, 1, 0);

	myDirtyDirs = false;
}

void Transform::UpdateModel()
{
	if (myDirtyDirs)
	{
		UpdateRot();
	}

	myModelM = glm::translate(glm::mat4(1), myPos);
	myModelM = myModelM * myRotM;
	myModelM = glm::scale(myModelM, myScale);
	myModelM = glm::translate(myModelM, -myCenter);

	myDirtyModel = false;
}

// the quaternion needed to rotate v1 so that it matches v2
// thank you http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat Transform::RotationBetweenVectors(glm::vec3 aStart, glm::vec3 aDest)
{
	aStart = glm::normalize(aStart);
	aDest = glm::normalize(aDest);

	float cosTheta = glm::dot(aStart, aDest);
	glm::vec3 rotationAxis;

	if (cosTheta == -1) 
	{
		// special case when vectors in opposite directions:
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), aStart);
		if (length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), aStart);
		}

		rotationAxis = normalize(rotationAxis);
		return angleAxis(180.f, rotationAxis);
	}

	rotationAxis = glm::cross(aStart, aDest);

	float s = sqrt((1 + cosTheta) * 2);
	float invS = 1 / s;

	return glm::quat(
		s * 0.5f,
		rotationAxis.x * invS,
		rotationAxis.y * invS,
		rotationAxis.z * invS
	);
}