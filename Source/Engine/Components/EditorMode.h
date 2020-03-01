#pragma once

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(PhysicsWorld& aWorld);

	void Update(float aDeltaTime, PhysicsWorld& aWorld);

private:
	const float myMouseSensitivity;
	float myFlightSpeed;

	std::shared_ptr<PhysicsShapeBase> myPhysShape;
};