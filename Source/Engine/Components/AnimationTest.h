#pragma once

#include "../GameObject.h"

class Game;
class Skeleton;
class AnimationClip;
class GLTFImporter;

class AnimationTest
{
public:
	AnimationTest(Game& aGame);

	void Update(float aDeltaTime);

private:
	Handle<Model> GenerateModel(const Skeleton& aSkeleton);

	Game& myGame;
	Handle<GameObject> myGO;
	Handle<AnimationClip> myClip;
};