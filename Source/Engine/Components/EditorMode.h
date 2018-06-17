#pragma once

#include "ComponentBase.h"

class EditorMode : public ComponentBase
{
public:
	void Update(float deltaTime) override;

private:
	const float myMouseSensitivity = 3.f;
	const float myFlightSpeed = 2.f;
};