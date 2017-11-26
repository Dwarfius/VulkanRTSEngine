#include "Common.h"
#include "Components/Tank.h"

#include "Game.h"
#include "Audio.h"
#include "GameObject.h"
#include "Terrain.h"

#include "Components/GameMode.h"
#include "Components/Missile.h"
#include "Components/Renderer.h"

const int Tank::Type = 1;

void Tank::Update(float deltaTime)
{
	const float moveSpeed = 2.f;

	Transform *ownTransf = owner->GetTransform();
	vec3 pos = ownTransf->GetPos();
	vec3 diff = navTarget - pos;
	diff.y = 0; // we don't care about height difference
	if (length2(diff) < 1)
		owner->Die();

	pos += normalize(diff) * moveSpeed * deltaTime;

	float yOffset = owner->GetCenter().y * ownTransf->GetScale().y;
	const Terrain *terrain = Game::GetInstance()->GetTerrain(ownTransf->GetPos());
	pos.y = terrain->GetHeight(pos) + yOffset;
	ownTransf->SetPos(pos);

	ownTransf->LookAt(navTarget);
	ownTransf->RotateToUp(terrain->GetNormal(pos));

	// shoot logic
	if (shootTimer > 0)
		shootTimer -= deltaTime;
	
	if (shootTimer <= 0)
	{
		const float force = 10;
		vec3 forward = ownTransf->GetForward();

		vec3 spawnPos = pos + forward * 0.6f + vec3(0, 0.25f, 0);
		GameObject *go = Game::GetInstance()->Instantiate(spawnPos, vec3(), vec3(0.1f));
		if (go)
		{
			go->AddComponent(new Renderer(3, 0, 6));
			vec3 shootDir = forward + vec3(0, 0.2f, 0);
			go->AddComponent(new Missile(shootDir * force, owner, team));

			shootTimer = shootRate;
		}
	}
}

void Tank::Destroy()
{
	if (onDeathCallback && Game::GetInstance()->IsRunning())
		onDeathCallback(this);
	onDeathCallback = nullptr;
}