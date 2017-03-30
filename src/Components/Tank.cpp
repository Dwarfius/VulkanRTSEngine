#include "Components\Tank.h"
#include "Game.h"

Tank::Tank()
{
	navTarget = vec3(rand() % 100 - 50, 0, rand() % 100 - 50);
}

void Tank::Update(float deltaTime)
{
	const float moveSpeed = 1;

	Transform *ownTransf = owner->GetTransform();
	vec3 pos = ownTransf->GetPos();
	if (length2(navTarget - pos) < 1)
		navTarget = vec3(rand() % 100 - 50, 0, rand() % 100 - 50);

	pos += normalize(navTarget - pos) * moveSpeed * deltaTime;

	float yOffset = owner->GetCenter().y * ownTransf->GetScale().y;
	Terrain *terrain = Game::GetInstance()->GetTerrain(ownTransf->GetPos());
	pos.y = terrain->GetHeight(pos) + yOffset;
	ownTransf->SetPos(pos);

	ownTransf->LookAt(navTarget);
	ownTransf->RotateToUp(terrain->GetNormal(pos));
}

void Tank::OnCollideWithGO(GameObject *other)
{

}