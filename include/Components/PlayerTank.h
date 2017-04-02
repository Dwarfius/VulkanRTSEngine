#ifndef _PLAYER_TANK_H
#define _PLAYER_TANK_H

#include "Components\ComponentBase.h"
#include <glm\vec3.hpp>

using namespace glm;

class PlayerTank : public ComponentBase
{
public:
	void Update(float deltaTime) override;

private:
	const float shootRate = 1;
	float shootTimer = 0;

	float heightOffset = 0;
	vec3 angles;
};

#endif