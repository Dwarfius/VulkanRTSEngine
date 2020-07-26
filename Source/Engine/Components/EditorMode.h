#pragma once

#include "../Systems/ProfilerUI.h"

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(PhysicsWorld& aWorld);

	void Update(Game& aGame, float aDeltaTime, PhysicsWorld& aWorld);

private:
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);

	const float myMouseSensitivity;
	float myFlightSpeed;
	bool myDemoWindowVisible;

	std::shared_ptr<PhysicsShapeBase> myPhysShape;
	ProfilerUI myProfilerUI;
};