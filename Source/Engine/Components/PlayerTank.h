#pragma once

#include "ComponentBase.h"

class PlayerTank : public ComponentBase
{
public:
	void Update(float deltaTime) override;

private:
	const float shootRate = 1;
	float shootTimer = 0;

	float heightOffset = 0;
	glm::vec3 angles;
	float holdStart = -1;
};