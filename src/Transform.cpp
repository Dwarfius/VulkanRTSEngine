#include "Transform.h"
#include <string>

Transform::Transform()
{
	dirtyModel = dirtyDirs = true;

	pos = vec3(0, 0, 0);
	size = vec3(1, 1, 1);
	quaternion = quat(vec3(0, 0, 0));
}

void Transform::LookAt(vec3 target)
{
	forward = normalize(target - pos);
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
	quaternion = rot2 * rot1; // remember, in reverse order.

	UpdateRot();
}

void Transform::UpdateRot()
{
	// making sure that it's a proper unit quat
	quaternion = normalize(quaternion);
	rotM = mat4_cast(quaternion);

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

	modelM = translate(mat4(1), pos);
	modelM = modelM * rotM;
	modelM = scale(modelM, size);
	modelM = translate(modelM, -center);

	dirtyModel = false;
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