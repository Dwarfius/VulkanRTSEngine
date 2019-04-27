#pragma once

class PhysicsWorld;
class GameObject;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(PhysicsWorld& aWorld);

	void Update(float aDeltaTime, const PhysicsWorld& aWorld) const;

private:
	const float myMouseSensitivity = 0.1f;
	const float myFlightSpeed = 2.f;

	GameObject* myBall;
};