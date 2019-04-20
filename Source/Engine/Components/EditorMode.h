#pragma once

class PhysicsWorld;

class EditorMode
{
public:
	void Update(float aDeltaTime, const PhysicsWorld& aWorld) const;

private:
	const float myMouseSensitivity = 0.1f;
	const float myFlightSpeed = 2.f;
};