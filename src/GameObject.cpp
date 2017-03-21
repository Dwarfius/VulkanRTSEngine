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

	if (renderer)
	{
		Shader::UniformValue val;
		val.m = transf.GetModelMatrix(center);
		uniforms["Model"] = val;
	}
}

float GameObject::GetRadius()
{
	if (!renderer)
		return 0;

	vec3 scale = transf.GetScale();
	float maxScale = max({ scale.x, scale.y, scale.z });
	float radius = Game::GetGraphics()->GetModelRadius(renderer->GetModel());
	return maxScale * radius;
}

void GameObject::AddComponent(ComponentBase *component)
{
	component->Init(this);
	components.push_back(component);
	renderer = dynamic_cast<Renderer*>(component);
	if (renderer)
		center = Game::GetGraphics()->GetModelCenter(renderer->GetModel());
}