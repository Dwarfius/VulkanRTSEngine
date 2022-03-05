#pragma once

#include <Core/RefCounted.h>

class Game;
class GameObject;

// Sets up and runs the Tank stress test:
// a "wall" of tanks descends towards the middle,
// shooting each other.
class StressTest
{
public:
	StressTest(Game& aGame);

	void Update(Game& aGame, float aDeltaTime);

private:
	Handle<GameObject> myTank;
	float myRotationAngle = 0.f;
};