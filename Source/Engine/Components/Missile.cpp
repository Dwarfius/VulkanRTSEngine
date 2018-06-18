#include "Common.h"
#include "Missile.h"

#include "../GameObject.h"
#include "../Audio.h"
#include "../Game.h"

#include "Tank.h"

const int Missile::Type = 2;

Missile::Missile(glm::vec3 vel, GameObject *shooter, bool ownerTeam)
{
	velocity = vel;
	this->shooter = shooter;
	team = ownerTeam;
}

void Missile::Update(float deltaTime)
{
	const float g = -9.81f;

	velocity += glm::vec3(0, g, 0) * deltaTime;
	owner->GetTransform().Translate(velocity * deltaTime);
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
		Tank* other = static_cast<Tank*>(go->GetComponent(Tank::Type));
		if (other && other->GetTeam() != team)
		{
			owner->Die();
			go->Die();
		}
	}
}