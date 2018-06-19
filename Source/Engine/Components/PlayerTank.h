#pragma once

#include "ComponentBase.h"

class PlayerTank : public ComponentBase
{
public:
	PlayerTank();

	void Update(float aDeltaTime) override;

private:
	float myShootTimer = 0;
	float myHeightOffset = 0;
	glm::vec3 myAngles;
	float myHoldStart;
};