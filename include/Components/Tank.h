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
	Tank();

	void Update(float deltaTime) override;
	void OnCollideWithGO(GameObject *other) override;
	void Destroy() override;

	void SetOnDeathCallback(function<void(ComponentBase*)> callback) { onDeathCallback = callback; }

private:
	vec3 navTarget;

	function<void(ComponentBase*)> onDeathCallback = nullptr;
};

#endif