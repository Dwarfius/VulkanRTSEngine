#ifndef _PLAYER_TANK_H
#define _PLAYER_TANK_H

#include "Components\ComponentBase.h"

class PlayerTank : public ComponentBase
{
	void Update(float deltaTime) override;
};

#endif