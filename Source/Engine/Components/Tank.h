#pragma once

#include "ComponentBase.h"

class Tank : public ComponentBase
{
public:
	Tank(bool newTeam) : team(newTeam) {}

	void Update(float deltaTime) override;
	void Destroy() override;

	void SetOnDeathCallback(function<void(ComponentBase*)> callback) { onDeathCallback = callback; }
	void SetNavTarget(glm::vec3 target) { navTarget = target; }
	bool GetTeam() { return team; }

	int GetComponentType() override { return Type; }
	const static int Type;

private:
	glm::vec3 navTarget;
	float shootTimer = 0;
	float shootRate = 1;
	bool team;

	function<void(ComponentBase*)> onDeathCallback = nullptr;
};