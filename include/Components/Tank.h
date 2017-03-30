#ifndef _TANK_H
#define _TANK_H

#include "Components\ComponentBase.h"
#include <glm\glm.hpp>

using namespace glm;

class Tank : public ComponentBase
{
public:
	Tank();

	void Update(float deltaTime) override;
	void OnCollideWithGO(GameObject *other) override;

private:
	vec3 navTarget;
};

#endif