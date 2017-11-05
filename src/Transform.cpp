#include "Common.h"
#include "Transform.h"

Transform::Transform()
	: dirtyModel(true)
	, dirtyDirs(true)
	, myPos(0)
	, myScale(0)
	, myRotation(vec3(0))
{
}

Transform::Transform(vec3 pos, vec3 rot, vec3 scale)
	: dirtyModel(true)
	, dirtyDirs(true)
	, myPos(pos)
	, myScale(scale)
	, myRotation(vec3(rot))
{

}

void Transform::LookAt(vec3 target)
{
	forward = normalize(target - myPos);
	if (length2(forward) < 0.0001f)
		return;

	// Recompute desiredUp so that it's perpendicular to the direction
	// You can skip that part if you really want to force desiredUp
	vec3 desiredUp(0, 1, 0);
	vec3 right = cross(forward, desiredUp);
	desiredUp = cross(right, forward);

	// Find the rotation between the front of the object (that we assume towards +Z,
	// but this depends on your model) and the desired direction
	quat rot1 = RotationBetweenVectors(vec3(0.0f, 0.0f, 1.0f), forward);
	// Because of the 1rst rotation, the up is probably completely screwed up. 
	// Find the rotation between the "up" of the rotated object, and the desired up
	vec3 newUp = rot1 * vec3(0.0f, 1.0f, 0.0f);
	quat rot2 = RotationBetweenVectors(newUp, desiredUp);

	// Apply them
	myRotation = rot2 * rot1; // remember, in reverse order.

	UpdateRot();
}

void Transform::RotateToUp(vec3 newUp)
{
	quat extraRot = RotationBetweenVectors(GetUp(), newUp);
	myRotation = extraRot * myRotation;
	UpdateRot();
}

void Transform::UpdateRot()
{
	// NOTE: to fix the roll accumulating for fps camera, should do later:
	// quat = orienX * quat * orienY;

	// making sure that it's a proper unit quat
	myRotation = normalize(myRotation);
	rotM = mat4_cast(myRotation);

	// why does this has to be inverted?
	right   = rotM * vec4(-1, 0, 0, 0);
	up      = rotM * vec4(0, 1, 0, 0);
	forward = rotM * vec4(0, 0, 1, 0);

	dirtyDirs = false;
}

void Transform::UpdateModel(vec3 center)
{
	if (dirtyDirs)
		UpdateRot();

	modelM = translate(mat4(1), myPos);
	modelM = modelM * rotM;
	modelM = scale(modelM, myScale);
	modelM = translate(modelM, -center);

	dirtyModel = false;
}

vec3 Transform::RotateAround(vec3 point, vec3 refPoint, vec3 angles)
{
	point -= refPoint;
	point = quat(angles) * point;
	return point + refPoint;
}

// the quaternion needed to rotate v1 so that it matches v2
// thank you http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
quat Transform::RotationBetweenVectors(vec3 start, vec3 dest) 
{
	start = normalize(start);
	dest = normalize(dest);

	float cosTheta = dot(start, dest);
	vec3 rotationAxis;

	if (cosTheta == -1) 
	{
		// special case when vectors in opposite directions:
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
		if (length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
			rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);

		rotationAxis = normalize(rotationAxis);
		return angleAxis(180.f, rotationAxis);
	}

	rotationAxis = cross(start, dest);

	float s = sqrt((1 + cosTheta) * 2);
	float invS = 1 / s;

	return quat(
		s * 0.5f,
		rotationAxis.x * invS,
		rotationAxis.y * invS,
		rotationAxis.z * invS
	);
}