#include "Common.h"
#include "Tank.h"

#include "../Game.h"
#include "../Audio.h"
#include "../GameObject.h"
#include "../Terrain.h"

#include "Missile.h"
#include "Renderer.h"

const int Tank::Type = 1;

void Tank::Update(float deltaTime)
{
	const float moveSpeed = 2.f;

	Transform& ownTransf = owner->GetTransform();
	glm::vec3 pos = ownTransf.GetPos();
	glm::vec3 diff = navTarget - pos;
	diff.y = 0; // we don't care about height difference
	if (glm::length2(diff) < 1)
	{
		// if we reached our destination - just die off
		owner->Die();
	}

	pos += glm::normalize(diff) * moveSpeed * deltaTime;

	float yOffset = owner->GetCenter().y * ownTransf.GetScale().y;
	const Terrain *terrain = Game::GetInstance()->GetTerrain(ownTransf.GetPos());
	pos.y = terrain->GetHeight(pos) + yOffset;
	ownTransf.SetPos(pos);

	ownTransf.LookAt(navTarget);
	ownTransf.RotateToUp(terrain->GetNormal(pos));

	// shoot logic
	if (shootTimer > 0)
		shootTimer -= deltaTime;
	
	if (shootTimer <= 0)
	{
		const float force = 10;
		glm::vec3 forward = ownTransf.GetForward();

		glm::vec3 spawnPos = pos + forward * 0.6f + glm::vec3(0, 0.25f, 0);
		GameObject *go = Game::GetInstance()->Instantiate(spawnPos, glm::vec3(), glm::vec3(0.1f));
		if (go)
		{
			go->AddComponent(new Renderer(3, 0, 6));
			glm::vec3 shootDir = forward + glm::vec3(0, 0.2f, 0);
			go->AddComponent(new Missile(shootDir * force, owner, team));

			shootTimer = shootRate;
		}
	}
}

void Tank::Destroy()
{
	if (onDeathCallback && Game::GetInstance()->IsRunning())
	{
		onDeathCallback(this);
	}
	onDeathCallback = nullptr;
}