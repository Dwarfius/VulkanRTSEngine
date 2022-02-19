#pragma once

#include <Engine/Animation/Skeleton.h>
#include <Engine/Animation/AnimationController.h>
#include <Engine/Animation/AnimationClip.h>
#include <Core/Pool.h>
#include <Engine/Resources/GLTFImporter.h>
#include <Engine/UIWidgets/TopBar.h>

#include "HexSolver.h"

class PhysicsWorld;
class GameObject;
class PhysicsShapeBase;
class Transform;
class Game;
class AnimationSystem;
class AnimationTest;

// Class used for testing and prototyping the engine
class EditorMode
{
public:
	EditorMode(Game& aGame);
	~EditorMode();

	void Update(Game& aGame, float aDeltaTime, PhysicsWorld* aWorld);

private:
	void HandleCamera(Transform& aCamTransf, float aDeltaTime);

	const float myMouseSensitivity = 0.1f;
	float myFlightSpeed = 2.f;

	std::shared_ptr<PhysicsShapeBase> myPhysShape;
	TopBar myTopBar;

	// Testing
	void AddTestSkeleton(Game& aGame);
	void UpdateTestSkeleton(Game& aGame, float aDeltaTime);
	void DrawBoneHierarchy(int aSkeletonIndex);
	void DrawBoneInfo(int aSkeletonIndex);
	
	Skeleton::BoneIndex mySelectedBone = Skeleton::kInvalidIndex;
	std::vector<GameObject*> myGOs;
	GLTFImporter myGLTFImporter;
	int mySelectedSkeleton = -1;
	int myAddSkeletonCount = 0;
	bool myShowSkeletonUI = false;
	AnimationTest* myAnimTest;

	Handle<GameObject> myGO;

	HexSolver solver;
};