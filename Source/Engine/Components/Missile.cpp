#include "Common.h"
#include "Missile.h"

#include "../GameObject.h"
#include "../Audio.h"
#include "../Game.h"

#include "Tank.h"

const int Missile::Type = 2;

Missile::Missile(glm::vec3 aVel, const GameObject* aShooter, bool anOwnerTeam)
	: myVelocity(aVel)
	, myShooter(aShooter)
	, myTeam(anOwnerTeam)
{
}

void Missile::Update(float aDeltaTime)
{
	const float g = -9.81f;

	myVelocity += glm::vec3(0, g, 0) * aDeltaTime;
	myOwner->GetTransform().Translate(myVelocity * aDeltaTime);
}

void Missile::OnCollideWithTerrain()
{
	myOwner->Die();
}

void Missile::OnCollideWithGO(GameObject* aGo)
{
	// avoid hitting ourselves
	if (aGo != myShooter)
	{
		Tank* other = static_cast<Tank*>(aGo->GetComponent(Tank::Type));
		if (other && other->GetTeam() != myTeam)
		{
			myOwner->Die();
			aGo->Die();
		}
	}
}