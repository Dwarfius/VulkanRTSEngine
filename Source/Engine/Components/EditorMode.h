#pragma once

#include "ComponentBase.h"

class PhysicsWorld;

class EditorMode : public ComponentBase
{
public:
	EditorMode();
	void Update(float aDeltaTime) override;

	void SetPhysWorld(const PhysicsWorld* aWorld) { myPhysWorld = aWorld; }

private:
	const float myMouseSensitivity = 0.1f;
	const float myFlightSpeed = 2.f;

	const PhysicsWorld* myPhysWorld;
};