#include "Components\Tank.h"
#include "Game.h"
#include "Components\GameMode.h"
#include "Audio.h"

Tank::Tank()
{
	navTarget = vec3(rand() % 100 - 50, 0, rand() % 100 - 50);
}

void Tank::Update(float deltaTime)
{
	const float moveSpeed = 2.f;

	Transform *ownTransf = owner->GetTransform();
	vec3 pos = ownTransf->GetPos();
	vec3 diff = navTarget - pos;
	diff.y = 0; // we don't care about height difference
	if (length2(diff) < 1)
		navTarget = vec3(rand() % 100 - 50, 0, rand() % 100 - 50);

	pos += normalize(diff) * moveSpeed * deltaTime;

	float yOffset = owner->GetCenter().y * ownTransf->GetScale().y;
	Terrain *terrain = Game::GetInstance()->GetTerrain(ownTransf->GetPos());
	pos.y = terrain->GetHeight(pos) + yOffset;
	ownTransf->SetPos(pos);

	ownTransf->LookAt(navTarget);
	ownTransf->RotateToUp(terrain->GetNormal(pos));
}

void Tank::OnCollideWithGO(GameObject *other)
{
	GameMode *mode = GameMode::GetInstance();
	if (mode && other == mode->GetOwner())
	{
		owner->Die();
		other->Die();
	}
}

void Tank::Destroy()
{
	if (onDeathCallback && Game::GetInstance()->IsRunning())
	{
		Audio::Play(0, owner->GetTransform()->GetPos());
		onDeathCallback(this);
	}
	onDeathCallback = nullptr;
}