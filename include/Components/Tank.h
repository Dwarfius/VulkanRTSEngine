#ifndef _TANK_H
#define _TANK_H

#include "Components\ComponentBase.h"
#include <functional>
#include <glm\glm.hpp>

using namespace glm;
using namespace std;

class Tank : public ComponentBase
{
public:
	Tank(bool newTeam) : team(newTeam) {}

	void Update(float deltaTime) override;
	void Destroy() override;

	void SetOnDeathCallback(function<void(ComponentBase*)> callback) { onDeathCallback = callback; }
	void SetNavTarget(vec3 target) { navTarget = target; }
	bool GetTeam() { return team; }

	int GetComponentType() override { return Type; }
	const static int Type;

private:
	vec3 navTarget;
	float shootTimer = 0;
	float shootRate = 1;
	bool team;

	function<void(ComponentBase*)> onDeathCallback = nullptr;
};

#endif