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

	UpdateRot();
}

void Transform::UpdateRot()
{
	quaternion = quat(radians(euler));
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