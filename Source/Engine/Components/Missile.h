#pragma once

#include "ComponentBase.h"

class GameObject;

class Missile : public ComponentBase
{
public:
	Missile(glm::vec3 vel, GameObject *shooter, bool ownerTeam);

	void Update(float deltaTime) override;
	void OnCollideWithTerrain() override;
	void OnCollideWithGO(GameObject *go) override;

	int GetComponentType() override { return Type; }
	const static int Type;

private:
	glm::vec3 velocity;
	GameObject *shooter;
	bool team;
};