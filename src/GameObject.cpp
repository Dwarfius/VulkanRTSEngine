#include "GameObject.h"
#include "Game.h"
#include "Common.h"

GameObject::~GameObject()
{
	for (auto comp : components)
	{
		comp->Destroy();
		delete comp;
	}
	components.clear();
}

void GameObject::Update(float deltaTime)
{
	for (auto comp : components)
		comp->Update(deltaTime);
}

void GameObject::UpdateMatrix()
{
	if (renderer == nullptr || !modelDirty)
		return;

	vec3 center = Game::GetGraphics()->GetModelCenter(renderer->GetModel());

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

void GameObject::AddComponent(ComponentBase *component)
{
	component->Init(this);
	components.push_back(component);
	renderer = dynamic_cast<Renderer*>(component);
}