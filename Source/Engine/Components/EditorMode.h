#pragma once

#include "ComponentBase.h"

class EditorMode : public ComponentBase
{
public:
	void Update(float aDeltaTime) override;

private:
	const float myMouseSensitivity = 0.1f;
	const float myFlightSpeed = 2.f;
};