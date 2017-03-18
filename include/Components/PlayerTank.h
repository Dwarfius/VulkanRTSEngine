#ifndef _PLAYER_TANK_H
#define _PLAYER_TANK_H

#include "Components\ComponentBase.h"

class PlayerTank : public ComponentBase
{
public:
	void Update(float deltaTime) override;

private:
	float heightOffset = 0;
};

#endif