#include "Components\Missile.h"
#include "GameObject.h"

Missile::Missile(vec3 vel, GameObject *shooter)
{
	velocity = vel;
	this->shooter = shooter;
}

void Missile::Update(float deltaTime)
{
	const float g = -9.81f;

	velocity += vec3(0, g, 0) * deltaTime;
	owner->GetTransform()->Translate(velocity * deltaTime);
}

void Missile::OnCollideWithTerrain()
{
	owner->Die();
}

void Missile::OnCollideWithGO(GameObject *go)
{
	// avoid hitting ourselves
	if (go != shooter)
	{
		owner->Die();
		go->Die();
	}
}