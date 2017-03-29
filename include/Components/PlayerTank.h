#ifndef _PLAYER_TANK_H
#define _PLAYER_TANK_H

#include "Components\ComponentBase.h"

class PlayerTank : public ComponentBase
{
public:
	void Update(float deltaTime) override;
	void OnCollideWithTerrain() override;
	void OnCollideWithGO(GameObject *other) override;

private:
	float heightOffset = 0;
};

#endif