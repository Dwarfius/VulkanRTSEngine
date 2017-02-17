#include "GameObject.h"
#include "Game.h"
#include "Common.h"

void GameObject::Update(float deltaTime)
{
	
}

void GameObject::UpdateMatrix()
{
	if (!modelDirty)
		return;

	vec3 center = Game::GetGraphics()->GetModelCenter(model);

	modelMatrix = translate(mat4(1), pos);
	modelMatrix = rotate(modelMatrix, radians(rotation.x), vec3(1, 0, 0));
	modelMatrix = rotate(modelMatrix, radians(rotation.y), vec3(0, 1, 0));
	modelMatrix = rotate(modelMatrix, radians(rotation.z), vec3(0, 0, 1));
	modelMatrix = scale(modelMatrix, size);
	modelMatrix = translate(modelMatrix, -center);

	Shader::UniformValue val;
	val.m = modelMatrix;
	uniforms["model"] = val;

	modelDirty = false;
}