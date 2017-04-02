#ifndef _MISSILE_H
#define _MISSILE_H

#include "Components\ComponentBase.h"
#include <glm\vec3.hpp>

using namespace glm;

class GameObject;

class Missile : public ComponentBase
{
public:
	Missile(vec3 vel, GameObject *shooter);

	void Update(float deltaTime) override;
	void OnCollideWithTerrain() override;
	void OnCollideWithGO(GameObject *go) override;

private:
	vec3 velocity;
	GameObject *shooter;
};

#endif