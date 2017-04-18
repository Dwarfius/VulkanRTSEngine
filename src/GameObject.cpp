#include "GameObject.h"
#include "Game.h"
#include "Common.h"

GameObject::GameObject(vec3 pos, vec3 rot, vec3 scale)
{
	transf.SetPos(pos);
	transf.SetRotation(rot);
	transf.SetScale(scale);
}

GameObject::~GameObject()
{
	for (auto comp : components)
	{
		comp->Destroy();
		delete comp;
	}
	components.clear();

	Game::GetInstance()->ReturnId(index);
}

void GameObject::Update(float deltaTime)
{
	for (int i=0; i<components.size(); i++)
		components[i]->Update(deltaTime);

	if (renderer)
	{
		Shader::UniformValue val;
		val.m = transf.GetModelMatrix(center);
		uniforms["Model"] = val;
	}
}

float GameObject::GetRadius() const
{
	if (!renderer)
		return 0;

	const vec3 scale = transf.GetScale();
	const float maxScale = max({ scale.x, scale.y, scale.z });
	const float radius = Game::GetGraphics()->GetModelRadius(renderer->GetModel());
	return maxScale * radius;
}

void GameObject::AddComponent(ComponentBase *component)
{
	component->Init(this);
	components.push_back(component);
	Renderer *newRenderer = dynamic_cast<Renderer*>(component);
	if (newRenderer && !renderer)
	{
		renderer = newRenderer;
		center = Game::GetGraphics()->GetModelCenter(renderer->GetModel());
	}
	else if (newRenderer && renderer)
		printf("[Warning] Attempting to attach a renderer to a component with a renderer, ignoring\n");
}

ComponentBase* GameObject::GetComponent(int type) const
{
	for (ComponentBase* comp : components)
	{
		if (comp->GetComponentType() == type)
			return comp;
	}
	return nullptr;
}

void GameObject::CollidedWithTerrain()
{
	if (collidedWithTerrain)
		return;

	collidedWithTerrain = true;
	for (ComponentBase *base : components)
		base->OnCollideWithTerrain();
}

void GameObject::CollidedWithGO(GameObject *go)
{
	if (objsCollidedWith.count(go))
		return;

	objsCollidedWith.insert(go);
	for (ComponentBase *base : components)
		base->OnCollideWithGO(go);
}

void GameObject::PreCollision()
{
	collidedWithTerrain = false;
	objsCollidedWith.clear();
}

bool GameObject::Collide(GameObject *g1, GameObject *g2)
{
	const vec3 g1Pos = g1->GetTransform()->GetPos();
	const vec3 g2Pos = g2->GetTransform()->GetPos();

	const float g1Rad = g1->GetRadius();
	const float g2Rad = g2->GetRadius();

	// simple bounding sphere check
	// using squares to avoid expensive sqrt
	const float radiiSum = g2Rad * 0.5f + g1Rad * 0.5f; // halving it because the models used have huge bounding spheres (antenna messes it up)
	return length2(g2Pos - g1Pos) < radiiSum * radiiSum;
}