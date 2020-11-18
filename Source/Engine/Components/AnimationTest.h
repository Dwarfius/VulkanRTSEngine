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
	~AnimationTest();

	void Update(float aDeltaTime);

private:
	Handle<Model> GenerateModel(const Skeleton& aSkeleton);

	Game& myGame;
	GameObject* myGO = nullptr;
	Handle<AnimationClip> myClip;
};