#pragma once

#include "ComponentBase.h"

class GameObject;

class Missile : public ComponentBase
{
public:
	Missile(glm::vec3 aVel, const GameObject* aShooter, bool anOwnerTeam);

	void Update(float aDeltaTime) override;
	void OnCollideWithTerrain() override;
	void OnCollideWithGO(GameObject* aGo) override;

	int GetComponentType() override { return Type; }
	const static int Type;

private:
	glm::vec3 myVelocity;
	const GameObject* myShooter;
	bool myTeam;
};