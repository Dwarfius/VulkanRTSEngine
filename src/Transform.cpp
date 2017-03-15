#include "Transform.h"

Transform::Transform()
{
	dirtyModel = dirtyDirs = true;

	yaw = -90;
	pitch = 0;
	roll = 0;
	pos = vec3(0, 0, 0);
	up = vec3(0, 1, 0);
	size = vec3(1, 1, 1);
}

void Transform::LookAt(vec3 target)
{
	forward = normalize(target - pos);
	pitch = degrees(asin(-forward.y));
	yaw = degrees(atan2(forward.z, forward.x));
	roll = 0;

	UpdateRot();
}

void Transform::UpdateRot()
{
	rotM = rotate(mat4(1), radians(-yaw), vec3(0, 1, 0));
	rotM = rotate(rotM, radians(-pitch), vec3(1, 0, 0));
	rotM = rotate(rotM, radians(roll), vec3(0, 0, 1));

	forward = rotM * vec4(0, 0, 1, 0);
	right = rotM * vec4(1, 0, 0, 0);
	up = rotM * vec4(0, 1, 0, 0);

	dirtyDirs = false;
}

void Transform::UpdateModel()
{
	if (dirtyDirs)
		UpdateRot();

	modelM = translate(mat4(1), pos);
	modelM = modelM * rotM;
	modelM = scale(modelM, size);

	dirtyModel = false;
}