#include "Common.h"
#include "Transform.h"

Transform::Transform()
	: dirtyModel(true)
	, dirtyDirs(true)
	, myPos(0)
	, myScale(0)
	, myRotation(glm::vec3(0))
{
}

Transform::Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
	: dirtyModel(true)
	, dirtyDirs(true)
	, myPos(pos)
	, myScale(scale)
	, myRotation(glm::vec3(rot))
{

}

void Transform::LookAt(glm::vec3 target)
{
	forward = glm::normalize(target - myPos);
	if (glm::length2(forward) < 0.0001f)
	{
		return;
	}

	// Recompute desiredUp so that it's perpendicular to the direction
	// You can skip that part if you really want to force desiredUp
	glm::vec3 desiredUp(0, 1, 0);
	glm::vec3 right = glm::cross(forward, desiredUp);
	desiredUp = glm::cross(right, forward);

	// Find the rotation between the front of the object (that we assume towards +Z,
	// but this depends on your model) and the desired direction
	glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 0.0f, 1.0f), forward);
	// Because of the 1rst rotation, the up is probably completely screwed up. 
	// Find the rotation between the "up" of the rotated object, and the desired up
	glm::vec3 newUp = rot1 * glm::vec3(0.0f, 1.0f, 0.0f);
	glm::quat rot2 = RotationBetweenVectors(newUp, desiredUp);

	// Apply them
	myRotation = rot2 * rot1; // remember, in reverse order.

	UpdateRot();
}

void Transform::RotateToUp(glm::vec3 newUp)
{
	glm::quat extraRot = RotationBetweenVectors(GetUp(), newUp);
	myRotation = extraRot * myRotation;
	UpdateRot();
}

void Transform::UpdateRot()
{
	// making sure that it's a proper unit quat
	myRotation = normalize(myRotation);
	rotM = mat4_cast(myRotation);

	// why does this has to be inverted?
	right   = rotM * glm::vec4(-1, 0, 0, 0);
	up      = rotM * glm::vec4(0, 1, 0, 0);
	forward = rotM * glm::vec4(0, 0, 1, 0);

	dirtyDirs = false;
}

void Transform::UpdateModel(glm::vec3 center)
{
	if (dirtyDirs)
		UpdateRot();

	modelM = translate(glm::mat4(1), myPos);
	modelM = modelM * rotM;
	modelM = scale(modelM, myScale);
	modelM = translate(modelM, -center);

	dirtyModel = false;
}

const glm::mat4& Transform::GetModelMatrix(glm::vec3 center)
{
	if (dirtyModel || dirtyDirs) 
	{ 
		UpdateModel(center); 
	} 
	return modelM;
}

glm::vec3 Transform::RotateAround(glm::vec3 point, glm::vec3 refPoint, glm::vec3 angles)
{
	point -= refPoint;
	point = glm::quat(angles) * point;
	return point + refPoint;
}

// the quaternion needed to rotate v1 so that it matches v2
// thank you http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat Transform::RotationBetweenVectors(glm::vec3 start, glm::vec3 dest)
{
	start = normalize(start);
	dest = normalize(dest);

	float cosTheta = dot(start, dest);
	glm::vec3 rotationAxis;

	if (cosTheta == -1) 
	{
		// special case when vectors in opposite directions:
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
		if (length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
		}

		rotationAxis = normalize(rotationAxis);
		return angleAxis(180.f, rotationAxis);
	}

	rotationAxis = cross(start, dest);

	float s = sqrt((1 + cosTheta) * 2);
	float invS = 1 / s;

	return glm::quat(
		s * 0.5f,
		rotationAxis.x * invS,
		rotationAxis.y * invS,
		rotationAxis.z * invS
	);
}