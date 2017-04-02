#include "Components\Missile.h"
#include "GameObject.h"
#include "Audio.h"

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
		Audio::Play(0, owner->GetTransform()->GetPos());

		owner->Die();
		go->Die();
	}
}