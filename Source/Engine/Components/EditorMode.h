#pragma once

class PhysicsWorld;
class GameObject;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(PhysicsWorld& aWorld);

	void Update(float aDeltaTime, const PhysicsWorld& aWorld);

private:
	const float myMouseSensitivity;
	float myFlightSpeed;

	GameObject* myBall;
};